#include <objscip/objscip.h>
#include <objscip/objscipdefplugins.h>
#include <scip/cons_linear.h>
#include <boost/graph/adjacency_list.hpp>
#include "graph.h"
#include <algorithm>
#include <iostream>
#include <cmath>

#include "pricer.h"
#include "vardata.h"
#include "connectivity_cons.h"

using namespace scip;

SegmentPricer::SegmentPricer(SCIP* scip, Graph& g_, std::vector<Graph::vertex_descriptor> master_nodes_, std::vector<SCIP_CONS*> partitioning_cons_, SCIP_CONS* num_segments_cons_) :
    ObjPricer(scip, "fitting_pricer", "description", 0, TRUE),
    g(g_), master_nodes(master_nodes_), partitioning_cons(partitioning_cons_), num_segments_cons(num_segments_cons_)
{}

SCIP_DECL_PRICERINIT(SegmentPricer::scip_init)
{
    for (size_t i = 0; i < partitioning_cons.size(); ++i)
    {
        SCIP_CALL(SCIPgetTransformedCons(scip, partitioning_cons[i], &partitioning_cons[i]));
    }
    SCIP_CALL(SCIPgetTransformedCons(scip, num_segments_cons, &num_segments_cons));
    
    _bigM = 0;
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (g[*p.first].color > _bigM)
        {
            _bigM = g[*p.first].color;
        }
    }
    _n = num_vertices(g);
    
    scip_pricers.resize(master_nodes.size());
    for (size_t i = 0; i < master_nodes.size(); ++i)
    {
        auto probdata = new PricerData();

        SCIP_CALL(SCIPcreate(&scip_pricers[i]));
        SCIP_CALL(SCIPincludeObjConshdlr(scip_pricers[i],
            new ConnectivityCons(scip_pricers[i], g, master_nodes, master_nodes[i], probdata->x),
            TRUE));
        SCIP_CALL(SCIPincludeDefaultPlugins(scip_pricers[i]));
        SCIPsetMessagehdlrQuiet(scip_pricers[i], TRUE);

        // create pricing problem
        SCIP_CALL(SCIPcreateObjProb(scip_pricers[i], "pricing_problem", probdata, TRUE));
        SCIP_CALL(SCIPsetObjsense(scip_pricers[i], SCIP_OBJSENSE_MINIMIZE));
    
        SCIP_CALL(setupVars(scip_pricers[i], master_nodes[i]));

        SCIP_CONS* cons;
        SCIP_CALL(SCIPcreateConsConnectivity(scip_pricers[i], &cons, "connectivity",
            TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE));
        SCIP_CALL(SCIPaddCons(scip_pricers[i], cons));
        SCIP_CALL(SCIPreleaseCons(scip_pricers[i], &cons));
    }
    return SCIP_OKAY;
}

SCIP_RETCODE SegmentPricer::setupVars(SCIP* scip_pricer, Graph::vertex_descriptor t)
{
    auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricer);

    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        SCIP_Real mu_s = 0.0; // random value, is set to the correct one at each iteration
        SCIP_VAR* x_s;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & x_s, "x_s", 0.0, 1.0, -mu_s, SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        if (*p.first == t)
        {
            SCIP_CALL(SCIPchgVarLb(scip_pricer, x_s, 1.0));
        }
        else if (std::find(master_nodes.begin(), master_nodes.end(), *p.first) != master_nodes.end())
        {
            // if a superpixel s is in T\{t}, then x_s must be 0
            SCIP_CALL(SCIPchgVarUb(scip_pricer, x_s, 0.0));
        }
        SCIP_CALL(SCIPaddVar(scip_pricer, x_s));
        probdata->x.push_back(x_s);
    }
    return SCIP_OKAY;
}

SCIP_DECL_PRICERREDCOST(SegmentPricer::scip_redcost)
{
    SCIP_Real lambda = SCIPgetDualsolLinear(scip, num_segments_cons);
    
    for (size_t i = 0; i < master_nodes.size(); ++i)
    {
        auto p = heuristic(scip, master_nodes[i], lambda); // returns pair<redcost, superpixels>
        if (SCIPisDualfeasNegative(scip, p.first))
        {
            std::cout << "heuristic successful: " << p.second.size() << std::endl;
            std::cout << "reduced costs: " << p.first << std::endl;
            SCIP_CALL(addPartitionVar(scip, master_nodes[i], p.second));
        }
        else
        {
            auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricers[i]);
            SCIP_CALL(SCIPfreeTransform(scip_pricers[i])); // reset transformation, solution data and SCIP stage
            for (auto s = vertices(g); s.first != s.second; ++s.first)
            {
                SCIP_Real mu_s = SCIPgetDualsolLinear(scip, partitioning_cons[*s.first]);
                SCIP_CALL(SCIPchgVarObj(scip_pricers[i], probdata->x[*s.first], -mu_s + std::abs(g[master_nodes[i]].color - g[*s.first].color)));
            }
            SCIP_CALL(SCIPsolve(scip_pricers[i]));
            SCIP_SOL* sol = SCIPgetBestSol(scip_pricers[i]);
            if (SCIPisDualfeasNegative(scip, SCIPgetSolOrigObj(scip_pricers[i], sol) - lambda))
            {
                //TODO compare SolOrigObj to sum -mu_s + |y_t - y_s|
                SCIP_CALL(addPartitionVarFromPricerSCIP(scip, scip_pricers[i], sol, master_nodes[i]));
            }
        }
    }
    *result = SCIP_SUCCESS; // at least one improving variable was found,
                            // or it is ensured that no such variable exists
    return SCIP_OKAY;
}

std::pair<SCIP_Real, std::vector<Graph::vertex_descriptor>> SegmentPricer::heuristic(SCIP* scip, Graph::vertex_descriptor master_node, SCIP_Real lambda)
{
    std::vector<Graph::vertex_descriptor> superpixels;
    SCIP_Real redcost = -SCIPgetDualsolLinear(scip, partitioning_cons[master_node]) - lambda;
    superpixels.push_back(master_node);
    while (superpixels.size() < _n - master_nodes.size() + 1)
    {
        SCIP_Real minimum = SCIPinfinity(scip);
        Graph::vertex_descriptor minimizer;
        for (auto superpixel : superpixels)
        {
            for (auto p = out_edges(superpixel, g); p.first != p.second; ++p.first)
            {
                assert(boost::source(*p.first, g) == superpixel);
                Graph::vertex_descriptor target = boost::target(*p.first, g);
                // the target should not already be in the segment
                if (std::find(superpixels.begin(), superpixels.end(), target) == superpixels.end()
                    && std::find(master_nodes.begin(), master_nodes.end(), target) == master_nodes.end())
                {
                    SCIP_Real mu_s = SCIPgetDualsolLinear(scip, partitioning_cons[target]);
                    if (SCIPisLT(scip, -mu_s + std::abs(g[master_node].color - g[target].color), minimum))
                    {
                        minimum = -mu_s + std::abs(g[master_node].color - g[target].color);
                        minimizer = target;
                    }
                }
            }
        }
        if (SCIPisNegative(scip, minimum)
            || !SCIPisDualfeasNegative(scip, redcost))
        {
            superpixels.push_back(minimizer);
            redcost += minimum;
        }
        else
        {
            break;
        }
    }
    return std::pair<SCIP_Real, std::vector<Graph::vertex_descriptor>>(redcost, superpixels);
}

SCIP_RETCODE SegmentPricer::addPartitionVarFromPricerSCIP(SCIP* scip, SCIP* scip_pricer, SCIP_SOL* sol, Graph::vertex_descriptor t)
{
    auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricer);
    SCIP_Real lambda = SCIPgetDualsolLinear(scip, num_segments_cons);

    std::vector<Graph::vertex_descriptor> superpixels;
    for (auto s = vertices(g); s.first != s.second; ++s.first)
    {
        if (SCIPisEQ(scip_pricer, SCIPgetSolVal(scip_pricer, sol, probdata->x[*s.first]), 1.0))
        {
            superpixels.push_back(*s.first);
        }
    }
    std::cout << "pricer successful: " << superpixels.size() << std::endl;
    std::cout << "reduced costs: " <<  SCIPgetSolOrigObj(scip_pricer, sol) - lambda << std::endl;
    SCIP_CALL(addPartitionVar(scip, t, superpixels));
    return SCIP_OKAY;
}

SCIP_RETCODE SegmentPricer::addPartitionVar(SCIP* scip, Graph::vertex_descriptor master_node, std::vector<Graph::vertex_descriptor> superpixels)
{
    SCIP_Real error_P = 0.0;
    for (auto s : superpixels)
    {
        error_P += std::abs(g[master_node].color - g[s].color);
    }

    auto vardata = new ObjVardataSegment(superpixels);
    SCIP_VAR* x_P;
    SCIP_CALL(SCIPcreateObjVar(scip, & x_P, "x_P", 0.0, 1.0, error_P, SCIP_VARTYPE_BINARY, FALSE, FALSE, vardata, TRUE));
    SCIP_CALL(SCIPaddPricedVar(scip, x_P, 1.0));

    // add coefficients to constraints (of the master problem)
    for (auto s : superpixels)
    {
        SCIP_CALL(SCIPaddCoefLinear(scip, partitioning_cons[s], x_P, 1.0));
    }
    SCIP_CALL(SCIPaddCoefLinear(scip, num_segments_cons, x_P, 1.0));

    SCIP_CALL(SCIPreleaseVar(scip, &x_P));

    return SCIP_OKAY;
}
