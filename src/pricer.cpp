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

using namespace scip;

ObjPricerLinFit::ObjPricerLinFit(SCIP* scip, Graph* g_, int k, std::vector<Graph::vertex_descriptor> T, std::vector<SCIP_CONS*> partitioning_cons, SCIP_CONS* num_partitions_cons) :
    ObjPricer(scip, "fitting_pricer", "description", 0, TRUE),
    g(g_), _k(k), _T(T), _partitioning_cons(partitioning_cons), _num_partitions_cons(num_partitions_cons)
{}

SCIP_DECL_PRICERINIT(ObjPricerLinFit::scip_init)
{
    for (size_t i = 0; i < _partitioning_cons.size(); ++i)
    {
        SCIP_CALL(SCIPgetTransformedCons(scip, _partitioning_cons[i], &_partitioning_cons[i]));
    }
    SCIP_CALL(SCIPgetTransformedCons(scip, _num_partitions_cons, &_num_partitions_cons));
    
    _bigM = 0;
    for (auto p = vertices(*g); p.first != p.second; ++p.first)
    {
        if ((*g)[*p.first].color > _bigM)
        {
            _bigM = (*g)[*p.first].color;
        }
    }
    _n = num_vertices(*g);
    
    scip_pricers.resize(_T.size());
    for (size_t i = 0; i < _T.size(); ++i)
    {
        SCIP_CALL(SCIPcreate(&scip_pricers[i]));
        SCIP_CALL(SCIPincludeDefaultPlugins(scip_pricers[i]));
        SCIPsetMessagehdlrQuiet(scip_pricers[i], TRUE);

        // create pricing problem
        auto probdata = new PricerData();
        SCIP_CALL(SCIPcreateObjProb(scip_pricers[i], "pricing_problem", probdata, TRUE));
        SCIP_CALL(SCIPsetObjsense(scip_pricers[i], SCIP_OBJSENSE_MINIMIZE));
    
        SCIP_CALL(setupVars(scip_pricers[i]));
        SCIP_CALL(setupConnectivityCons(scip_pricers[i], _T[i]));
    }
    
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::setupVars(SCIP* scip_pricer)
{
    auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricer);

    for (auto p = vertices(*g); p.first != p.second; ++p.first)
    {
        SCIP_Real mu_s = 0.0; // random value, is set to the correct one at each iteration
        SCIP_VAR* x_s;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & x_s, "x_s", 0.0, 1.0, -mu_s, SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, x_s));
        probdata->x.push_back(x_s);
    }
    
    // float variables for connectivity constraints
    probdata->e.resize(_n);
    for (size_t i = 0; i < _n; ++i)
    {
        probdata->e[i].resize(_n);
    }
    for (auto p = edges(*g); p.first != p.second; ++p.first)
    {
        auto source = boost::source(*p.first, *g);
        auto target = boost::target(*p.first, *g);

        SCIP_VAR* e_vw;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & e_vw, "e_vw", 0.0, SCIPinfinity(scip_pricer), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, e_vw));
        probdata->e[source][target] = e_vw;

        SCIP_VAR* e_wv;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & e_wv, "e_wv", 0.0, SCIPinfinity(scip_pricer), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, e_wv));
        probdata->e[target][source] = e_wv;
    }

    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::setupConnectivityCons(SCIP* scip_pricer, Graph::vertex_descriptor t)
{
    auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricer);

    // connectivity constraints (24)
    for (auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        SCIP_CONS* cons1;
        if (*s.first == t)
        {
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1, "first", 0, NULL, NULL, -1.0, -1.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        }
        else
        {
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1, "first", 0, NULL, NULL, 0.0, 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        }
 
        for (auto q = out_edges(*s.first, *g); q.first != q.second; ++q.first)
        {
            auto source = boost::source(*q.first, *g);
            assert(source == *s.first);
            auto target = boost::target(*q.first, *g);
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, probdata->e[source][target], 1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, probdata->e[target][source], -1.0));
        }

        if (*s.first == t)
        {
            for (auto q = vertices(*g); q.first != q.second; ++q.first)
            {
                SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, probdata->x[*q.first], -1.0));
            }
        }
        else
        {
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, probdata->x[*s.first], 1.0));
        }
        SCIP_CALL(SCIPaddCons(scip_pricer, cons1));
        SCIP_CALL(SCIPreleaseCons(scip_pricer, &cons1));
    }
    
    // connectivity constraints (25) and (26)        
    for (auto p = edges(*g); p.first != p.second; ++p.first)
    {
        auto source = boost::source(*p.first, *g);
        auto target = boost::target(*p.first, *g);
        
        SCIP_CONS* cons2;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, probdata->e[source][target], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, probdata->e[target][source], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, probdata->x[source], _k - _n));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons2));
        SCIP_CALL(SCIPreleaseCons(scip_pricer, &cons2));
        
        SCIP_CONS* cons3;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, probdata->e[source][target], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, probdata->e[target][source], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, probdata->x[target], _k - _n));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons3));
        SCIP_CALL(SCIPreleaseCons(scip_pricer, &cons3));
    }
    
    // connectivity constraints (27) and (28)
    // these optional and equivalent to (25) and (26)
    /* for (auto p = vertices(*g); p.first != p.second; ++p.first)
    {
        SCIP_CONS* cons4;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons4, "fourth", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        
        SCIP_CONS* cons5;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons5, "fifth", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        
        for (auto q = out_edges(*p.first, *g); q.first != q.second; ++q.first)
        {
            auto source = boost::source(*q.first, *g); 
            assert(source == *p.first);
            auto target = boost::target(*q.first, *g);
            
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons4, probdata->e[source][target], 1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons5, probdata->e[target][source], 1.0));
        }
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons4, probdata->x[*p.first], -_n + _k));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons4));
        SCIP_CALL(SCIPreleaseCons(scip_pricer, &cons4));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons5, probdata->x[*p.first], -_n + _k));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons5));
        SCIP_CALL(SCIPreleaseCons(scip_pricer, &cons5));
    } */
    
    return SCIP_OKAY;
}

SCIP_DECL_PRICERREDCOST(ObjPricerLinFit::scip_redcost)
{
    //std::cout << "enter pricer" << std::endl;
    SCIP_Real lambda = SCIPgetDualsolLinear(scip, _num_partitions_cons);
    
    for (size_t i = 0; i < _T.size(); ++i)
    {
        // try to find a new partition heuristically
        std::vector<Graph::vertex_descriptor> partition;
        SCIP_Real fitting_error;
        /*if (SCIPisNegative(scip, heuristic(scip, _T[i], partition, fitting_error) - lambda))
        {
            std::cout << "heuristic successful: " << partition.size() << std::endl;
            SCIP_CALL(addPartitionVar(scip, partition, fitting_error));
        }*/
        //else // solve the prcing problem exactly
        {
            auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricers[i]);

            SCIP_CALL(SCIPfreeTransform(scip_pricers[i])); // reset transformation, solution data and SCIP stage
            for (auto s = vertices(*g); s.first != s.second; ++s.first)
            {
                SCIP_Real mu_s = SCIPgetDualsolLinear(scip, _partitioning_cons[*s.first]);
                //std::cout << "mu_" << *s.first << ": " << mu_s << std::endl;
                //std::cout << (*g)[*s.first].color << std::endl;
                //std::cout << std::abs((*g)[_T[i]].color - (*g)[*s.first].color) << std::endl;
                SCIP_CALL(SCIPchgVarObj(scip_pricers[i], probdata->x[*s.first], -mu_s + std::abs((*g)[_T[i]].color - (*g)[*s.first].color)));
            }
            
            SCIP_CALL(SCIPsolve(scip_pricers[i]));
            SCIP_SOL* sol = SCIPgetBestSol(scip_pricers[i]);
            //std::cout << SCIPgetSolOrigObj(scip_pricers[i], sol) << std::endl;
            //std::cout << lambda << std::endl;
            if (SCIPisNegative(scip, SCIPgetSolOrigObj(scip_pricers[i], sol) - lambda))
            {
                SCIP_CALL(addPartitionVarFromPricerSCIP(scip, scip_pricers[i], sol, _T[i]));
            }
        }
    }
    
    *result = SCIP_SUCCESS; // at least one improving variable was found,
                            // or it is ensured that no such variable exists
    return SCIP_OKAY;
}

// returns the objective value
SCIP_Real ObjPricerLinFit::heuristic(SCIP* scip, Graph::vertex_descriptor t, std::vector<Graph::vertex_descriptor>& partition, SCIP_Real& fitting_error)
{
    partition = {t};
    std::vector<Graph::vertex_descriptor> to_examine = {t};
    SCIP_Real fitting_value = (*g)[t].color;
    fitting_error = 0.0;
    SCIP_Real objective_value = -SCIPgetDualsolLinear(scip, _partitioning_cons[t]);  // -mu_t
    while (!to_examine.empty())
    {
        Graph::vertex_descriptor s = to_examine.back();
        to_examine.pop_back();
        for (auto p = adjacent_vertices(s, *g); p.first != p.second; ++p.first)
        {
            SCIP_Real mu_s = SCIPgetDualsolLinear(scip, _partitioning_cons[*p.first]);
            if (SCIPisNegative(scip, -mu_s + std::abs(fitting_value - (*g)[*p.first].color)))
            {
                partition.push_back(*p.first);
                to_examine.push_back(*p.first);
                objective_value += -mu_s + std::abs(fitting_value - (*g)[*p.first].color);
                fitting_error += std::abs(fitting_value - (*g)[*p.first].color);
            }
        }
    }
    return objective_value;
}

SCIP_RETCODE ObjPricerLinFit::addPartitionVarFromPricerSCIP(SCIP* scip, SCIP* scip_pricer, SCIP_SOL* sol, Graph::vertex_descriptor t)
{
    auto probdata = (PricerData*) SCIPgetObjProbData(scip_pricer);

    SCIP_Real gamma_P = 0.0;
    for(auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        gamma_P += std::abs((*g)[t].color - (*g)[*s.first].color);
    }

    std::vector<Graph::vertex_descriptor> superpixels;
    for (auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        if (SCIPisZero(scip_pricer, SCIPgetSolVal(scip_pricer, sol, probdata->x[*s.first]) - 1.0))
        {
            superpixels.push_back(*s.first);
        }
    }
    std::cout << "pricer successful: " << superpixels.size() << std::endl;
    SCIP_CALL(addPartitionVar(scip, superpixels, gamma_P));
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::addPartitionVar(SCIP* scip, std::vector<Graph::vertex_descriptor> superpixels, SCIP_Real gamma_P)
{
    auto vardata = new ObjVardataSegment(superpixels);
    SCIP_VAR* x_P;
    SCIP_CALL(SCIPcreateObjVar(scip, & x_P, "x_P", 0.0, 1.0, gamma_P, SCIP_VARTYPE_BINARY, TRUE, FALSE, vardata, TRUE));
    SCIP_CALL(SCIPaddPricedVar(scip, x_P, 1.0));

    // add coefficients to constraints (of the master problem)
    for (auto s : superpixels)
    {
        SCIP_CALL(SCIPaddCoefLinear(scip, _partitioning_cons[s], x_P, 1.0));
    }
    SCIP_CALL(SCIPaddCoefLinear(scip, _num_partitions_cons, x_P, 1.0));

    return SCIP_OKAY;
}
