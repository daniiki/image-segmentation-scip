#include "graph.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>
#include "pricer.h"

#include <scip/scipdefplugins.h>

#include <iostream>
#include <math.h>

#include "vardata.h"

Graph create_graph()
{
    Graph g(12);
    std::vector<unsigned int> colors = {0,100,1,200,2,200,100,100,202,100,202,100};
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        g[*p.first].color = colors[*p.first];
    }
    add_edge(0,1,g);
    add_edge(0,2,g);
    add_edge(1,6,g);
    add_edge(2,3,g);
    add_edge(2,4,g);
    add_edge(3,5,g);
    add_edge(5,10,g);
    add_edge(6,7,g);
    add_edge(6,8,g);
    add_edge(8,9,g);
    add_edge(8,10,g);
    add_edge(9,11,g);
    return g;
}

// TO DO: correctly calculate fitting error
double gamma(Graph g, std::set<Graph::vertex_descriptor> partition)
{
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

SCIP_RETCODE master_problem(Graph g, int k, std::vector<Graph::vertex_descriptor> T, std::vector<std::set<Graph::vertex_descriptor>> inital_partitions)
{
    SCIP* scip;
    SCIP_CALL(SCIPcreate(& scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    
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
    ObjPricerLinFit* pricer_ptr = new ObjPricerLinFit(scip, &g, k, T, partitioning_cons, num_partitions_cons);
    SCIP_CALL(SCIPincludeObjPricer(scip, pricer_ptr, true));
    
    // activate pricer 
    SCIP_CALL(SCIPactivatePricer(scip, SCIPfindPricer(scip, "fitting_pricer")));
    
    // solve
    SCIP_CALL(SCIPsolve(scip));
    SCIP_SOL* sol = SCIPgetBestSol(scip);
    
    // print all variable values (of the variables x_P)
    SCIP_VAR** variables = SCIPgetVars(scip);
    for (int i = 0; i < SCIPgetNVars(scip); ++i)
    {
        std::cout << SCIPgetSolVal(scip, sol, variables[i]) << std::endl;
    }
    std::cout << std::endl;

    // print selected segments
    for (int i = 0; i < SCIPgetNVars(scip); ++i)
    {
        if (SCIPisZero(scip, SCIPgetSolVal(scip, sol, variables[i]) - 1.0))
        {
            auto vardata = (ObjVardataSegment*) SCIPgetObjVardata(scip, variables[i]);
            for (Graph::vertex_descriptor s : vardata->getSuperpixels())
            {
                std::cout << s << " ";
            }
            std::cout << std::endl;
        }
    }
    
    return SCIP_OKAY;
}

int main()
{
    auto g = create_graph();
    std::vector<Graph::vertex_descriptor> T = {4, 11, 10, 7};
    std::vector<std::set<Graph::vertex_descriptor>> inital_partitions = {{0,2,3,4,5}, {10}, {1, 6, 8, 9, 11}, {7}} ;
    master_problem(g, 4, T, inital_partitions);
    return 0;
}
