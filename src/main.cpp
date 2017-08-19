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

/**
 * Setup and solve the master problem
 */
SCIP_RETCODE master_problem(
    Graph& g,
    std::vector<Graph::vertex_descriptor> master_nodes,
    std::vector<std::set<Graph::vertex_descriptor>> initial_segments,
    std::vector<std::vector<Graph::vertex_descriptor>>& segments ///< the selected segments will be stored in here
)
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
    for (auto segment : initial_segments)
    {
        SCIP_VAR* var;
        // Set a very high objective value for the initial segments so that they aren't selected in the final solution
        SCIP_CALL(SCIPcreateVar(scip, &var, "x_P", 0.0, 1.0, 10000, SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
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
        for (size_t i = 0; i != initial_segments.size(); ++i)
        {
            if (initial_segments[i].find(*p.first) != initial_segments[i].end())
            {
                SCIP_CALL(SCIPaddCoefLinear(scip, cons1, vars[i], 1.0));
            }
        }
        SCIP_CALL(SCIPaddCons(scip, cons1));
        partitioning_cons.push_back(cons1);
    }
    
    SCIP_CONS* num_segments_cons;
    size_t k = initial_segments.size();
    SCIP_CALL(SCIPcreateConsLinear(scip, &num_segments_cons, "second", 0, NULL, NULL, k, k,
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
    for (size_t i = 0; i != initial_segments.size(); ++i)
    {
        SCIP_CALL(SCIPaddCoefLinear(scip, num_segments_cons, vars[i], 1.0));
    }
    SCIP_CALL(SCIPaddCons(scip, num_segments_cons));
    
    // include pricer 
    ObjPricerLinFit* pricer_ptr = new ObjPricerLinFit(scip, g, master_nodes, partitioning_cons, num_segments_cons);
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
            segments.push_back(vardata->getSuperpixels());
        }
    }

    // check if the selected segments are connected
    for (auto segment : segments)
    {
        Graph& subgraph = g.create_subgraph();
        std::vector<int> component(num_vertices(g));
        for (auto superpixel : segment)
        {
            add_vertex(superpixel, subgraph);
        }
        size_t num_components = connected_components(subgraph, &component[0]);
        assert(num_components == 1);
    }
    
    return SCIP_OKAY;
}

int main()
{
    Image image("src/input.png", 30);
    Graph g = image.graph();
    size_t n = num_vertices(g);
    size_t k = 5; // number of segments to cover the image with
    size_t m = n / k;
    std::vector<Graph::vertex_descriptor> master_nodes(k);
    std::vector<std::set<Graph::vertex_descriptor>> inital_segments(k);
    for (size_t i = 0; i < k; i++)
    {
        master_nodes[i] = i * m;
    }
    for (size_t i = 0; i < k - 1; i++)
    {
        for (size_t j = master_nodes[i]; j < master_nodes[i+1]; ++j)
        {
            inital_segments[i].insert(j);
        }
    }
    for (size_t j = master_nodes[k-1]; j < n; ++j)
    {
        inital_segments[k-1].insert(j);
    }

    std::vector<std::vector<Graph::vertex_descriptor>> segments; // the selected segments will be stored in here
    SCIP_CALL(master_problem(g, master_nodes, inital_segments, segments));
    image.writeSegments(master_nodes, segments, g);
    return 0;
}
