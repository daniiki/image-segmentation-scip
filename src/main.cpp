#include "graph.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/connected_components.hpp>
#include "pricer.h"

#include <scip/scipdefplugins.h>

#include <iostream>
#include <math.h>

#include "vardata.h"
#include "image.h"

// TO DO: correctly calculate fitting error
double gamma(Graph g, std::set<Graph::vertex_descriptor> partition)
{
    return 10000.0;
    double sum = 0;
    for (auto superpixel : partition)
    {
        sum += g[superpixel].color;
    }
    double average = sum / partition.size();
    
    double gamma = 0;
    for (auto superpixel : partition)
    {
        gamma += fabs(average - g[superpixel].color);
    }
    return gamma;
}

SCIP_RETCODE master_problem(Graph& g, int k, std::vector<Graph::vertex_descriptor> T, std::vector<std::set<Graph::vertex_descriptor>> inital_partitions, std::vector<std::vector<Graph::vertex_descriptor>>& partitions)
{
    SCIP* scip;
    SCIP_CALL(SCIPcreate(& scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    SCIP_CALL(SCIPsetIntParam(scip, "display/verblevel", 5));
    SCIP_CALL(SCIPsetIntParam(scip, "presolving/maxrestarts", 0)); // see Known Bugs at http://scip.zib.de/#contact
    
    //create master problem
    SCIP_CALL(SCIPcreateProb(scip, "master_problem", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE));
    
    std::vector<SCIP_VAR*> vars;
    // vars[i] belongs to partition partitions[i]
    for (auto partition : inital_partitions)
    {
        SCIP_VAR* var;
        SCIP_CALL(SCIPcreateVar(scip, & var, "x_P", 0.0, 1.0, gamma(g, partition), SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, var));
        vars.push_back(var);
    }
    
    std::vector<SCIP_CONS*> partitioning_cons;
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        SCIP_CONS* cons1;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons1, "first", 0, NULL, NULL, 1.0, 1.0,
                     true,                   /* initial */
                     false,                  /* separate */
                     true,                   /* enforce */
                     true,                   /* check */
                     true,                   /* propagate */
                     false,                  /* local */
                     true,                   /* modifiable */
                     false,                  /* dynamic */
                     false,                  /* removable */
                     false) );               /* stickingatnode */
        for (int i = 0; i != inital_partitions.size(); ++i)
        {
            if (inital_partitions[i].find(*p.first) != inital_partitions[i].end())
            {
                SCIP_CALL(SCIPaddCoefLinear(scip, cons1, vars[i], 1.0));
            }
        }
        SCIP_CALL(SCIPaddCons(scip, cons1));
        partitioning_cons.push_back(cons1);
    }
    
    SCIP_CONS* num_partitions_cons;
    SCIP_CALL(SCIPcreateConsLinear(scip, & num_partitions_cons, "second", 0, NULL, NULL, k, k,
                     true,                   /* initial */
                     false,                  /* separate */
                     true,                   /* enforce */
                     true,                   /* check */
                     true,                   /* propagate */
                     false,                  /* local */
                     true,                   /* modifiable */
                     false,                  /* dynamic */
                     false,                  /* removable */
                     false) );               /* stickingatnode */
    for (int i = 0; i != inital_partitions.size(); ++i)
    {
        SCIP_CALL(SCIPaddCoefLinear(scip, num_partitions_cons, vars[i], 1.0));
    }
    SCIP_CALL(SCIPaddCons(scip, num_partitions_cons));
    
    // include pricer 
    ObjPricerLinFit* pricer_ptr = new ObjPricerLinFit(scip, g, k, T, partitioning_cons, num_partitions_cons);
    SCIP_CALL(SCIPincludeObjPricer(scip, pricer_ptr, true));
    
    // activate pricer 
    SCIP_CALL(SCIPactivatePricer(scip, SCIPfindPricer(scip, "fitting_pricer")));
    
    // solve
    SCIP_CALL(SCIPsolve(scip));
    SCIP_SOL* sol = SCIPgetBestSol(scip);

    // return selected segments
    SCIP_VAR** variables = SCIPgetVars(scip);
    for (int i = 0; i < SCIPgetNVars(scip); ++i)
    {
        if (SCIPisEQ(scip, SCIPgetSolVal(scip, sol, variables[i]), 1.0))
        {
            auto vardata = (ObjVardataSegment*) SCIPgetObjVardata(scip, variables[i]);
            partitions.push_back(vardata->getSuperpixels());
        }
    }

    // check if the selected segments are connected
    for (auto segment : partitions)
    {
        Graph& subgraph = g.create_subgraph();
        std::vector<int> component(num_vertices(g));
        for (auto superpixel : segment)
        {
            add_vertex(superpixel, subgraph);
        }
        size_t num_components = connected_components(subgraph, &component[0]);
        std::cout << "Segment has " << num_components << " connected components." << std::endl;
    }
    
    return SCIP_OKAY;
}

int main()
{
    Image image("src/input.png", 20);
    Graph g = image.graph();
    int n = num_vertices(g);
    int k = 5;
    int m = n / k;
    std::vector<Graph::vertex_descriptor> T(k);
    std::vector<std::set<Graph::vertex_descriptor>> inital_partitions(k);
    for (int i = 0; i < k; i++)
    {
        T[i] = i * m;
    }
    for (int i = 0; i < k - 1; i++)
    {
        for (int j = T[i]; j < T[i+1]; ++j)
        {
            inital_partitions[i].insert(j);
        }
    }
    for (int j = T[k-1]; j < n; ++j)
    {
        inital_partitions[k-1].insert(j);
    }

    std::vector<std::vector<Graph::vertex_descriptor>> partitions;
    SCIP_CALL(master_problem(g, k, T, inital_partitions, partitions));
    image.writeSegments(T, partitions, g);
    return 0;
}
