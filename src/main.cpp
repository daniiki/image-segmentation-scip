#include "graph.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>
#include "pricer.h"

#include <scip/scipdefplugins.h>

#include <iostream>
#include <math.h>

Graph create_graph()
{
    Graph g(8);
    std::vector<unsigned int> colors = {1,2,3,100,101,102,200,202};
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        g[*p.first].color = colors[*p.first];
    }
    //make graph complete
    for (auto s = vertices(g); s.first != s.second; ++s.first)
    {
        for (auto t = vertices(g); t.first != t.second; ++t.first)
        {
            if (*s.first < *t.first)
            {
                add_edge(*s.first, *t.first, g);
            }
        }
    }
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
        SCIP_CALL(SCIPcreateVar(scip, & var, "x_P", 0.0, 1.0, gamma(g, partition), SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
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
    
    return SCIP_OKAY;
}

int main()
{
    auto g = create_graph();
    std::vector<Graph::vertex_descriptor> T = {0, 3, 6};
    std::vector<std::set<Graph::vertex_descriptor>> inital_partitions = {{0}, {3}, {1,2,4,5,6,7}};
    master_problem(g, 3, T, inital_partitions);
    return 0;
}
