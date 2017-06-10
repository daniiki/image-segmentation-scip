#include <objscip/objscip.h>
#include <objscip/objscipdefplugins.h>
#include <scip/cons_linear.h>
#include <boost/graph/adjacency_list.hpp>
#include <algorithm>
#include <iostream>

#include "pricer.h"

using namespace scip;

ObjPricerLinFit::ObjPricerLinFit(SCIP* scip, Graph* g_, int k, std::vector<Graph::vertex_descriptor> T, std::vector<SCIP_CONS*> partitioning_cons, SCIP_CONS* num_partitions_cons) :
    ObjPricer(scip, "name", "description", 0, TRUE),
    g(g_), _k(k), _T(T), _partitioning_cons(partitioning_cons), _num_partitions_cons(num_partitions_cons)
{
    _bigM = 0;
    for (auto p = vertices(*g); p.first != p.second; ++p.first)
    {
        if ((*g)[*p.first].color > _bigM)
        {
            _bigM = (*g)[*p.first].color;
        }
    }
    _n = num_vertices(*g);
    
    initialSetup();
}

SCIP_RETCODE ObjPricerLinFit::initialSetup()
{
    SCIP_CALL(SCIPcreate(& scip_pricer));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip_pricer));
    
    // create pricing problem
    SCIP_CALL(SCIPcreateProb(scip_pricer, "pricing_problem", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPsetObjsense(scip_pricer, SCIP_OBJSENSE_MINIMIZE));
    
    SCIP_CALL(setupVars());
    SCIP_CALL(setupCons());
    SCIP_CALL(setupConnectivityCons());
    
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::setupVars()
{
    for (auto p = vertices(*g); p.first != p.second; ++p.first)
    {
        SCIP_Real mu_s = 0.0; // random value, is set to the correct one at each iteration
        SCIP_VAR* x_s;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & x_s, "x_s", 0.0, 1.0, -mu_s, SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, x_s));
        x.push_back(x_s);
        
        SCIP_VAR* delta_s;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & delta_s, "delta_s", 0.0, SCIPinfinity(scip_pricer), 1.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, delta_s));
        delta.push_back(delta_s);
    }
    
    SCIP_CALL(SCIPcreateVar(scip_pricer, & c_P, "c_P", -SCIPinfinity(scip_pricer), SCIPinfinity(scip_pricer), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddVar(scip_pricer, c_P));
    
    // float variables for connectivity constraints
    for (auto p = edges(*g); p.first != p.second; ++p.first)
    {
        SCIP_VAR* e_vw;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & e_vw, "e_vw", 0.0, SCIPinfinity(scip_pricer), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, e_vw));
        e1[*p.first] = e_vw;
        
        SCIP_VAR* e_wv;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & e_wv, "e_wv", 0.0, SCIPinfinity(scip_pricer), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, e_wv));
        e2[*p.first] = e_wv;
    }
    
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::setupCons()
{
    for (auto p = vertices(*g); p.first != p.second; ++p.first)
    {
        SCIP_CONS* cons1;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1, "first", 0, NULL, NULL, -SCIPinfinity(scip_pricer), _bigM + (*g)[*p.first].color, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, x[*p.first], _bigM));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, delta[*p.first], -1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, c_P, 1.0));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons1));
        
        SCIP_CONS* cons2;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip_pricer), _bigM - (*g)[*p.first].color, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, x[*p.first], _bigM));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, delta[*p.first], -1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, c_P, -1.0));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons2));
        
        SCIP_CONS* cons3;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, x[*p.first], -_bigM));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, delta[*p.first], 1.0));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons3));
    }
    
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::setupConnectivityCons()
{
    // connectivity constraints (24) for s not in T
    for (auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        // if s is not in T
        if (std::find(_T.begin(), _T.end(), *s.first) == _T.end())
        {
            SCIP_CONS* cons1_s;
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1_s, "first", 0, NULL, NULL, 0.0, 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            
            for (auto q = out_edges(*s.first, *g); q.first != q.second; ++q.first)
            {
                auto source = boost::source(*q.first, *g);
                auto target = boost::target(*q.first, *g);
                if (source == *s.first)
                {
                    SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, e1[*q.first], 1.0));
                    SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, e2[*q.first], -1.0));
                }
                else if (target == *s.first)
                {
                    SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, e2[*q.first], 1.0));
                    SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, e1[*q.first], -1.0));
                }
                else
                {
                    std::cerr << "häääää?" << std::endl;
                    exit(1);
                }
            }
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, x[*s.first], 1.0));
        }
    }
    
    // connectivity constraints (25) and (26)        
    for (auto p = edges(*g); p.first != p.second; ++p.first)
    {
        auto source = boost::source(*p.first, *g);
        auto target = boost::target(*p.first, *g);
        
        SCIP_CONS* cons2;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, e1[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, e2[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, x[source], _k - _n));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons2));
        
        SCIP_CONS* cons3;
        SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, e1[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, e2[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, x[target], _k - _n));
        SCIP_CALL(SCIPaddCons(scip_pricer, cons3));
    }
    
    return SCIP_OKAY;
}

SCIP_DECL_PRICERREDCOST(ObjPricerLinFit::scip_redcost)
{
    for (auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        auto mu_s = SCIPgetDualsolLinear(scip, _partitioning_cons[*s.first]);
        SCIPchgVarObj(scip_pricer, x[*s.first], -mu_s);
    }
        
    for (auto t : _T)
    {
        addRemainingConnectivityCons(t);
        SCIP_CALL(SCIPsolve(scip_pricer));
        SCIP_SOL* sol = SCIPgetBestSol(scip_pricer);
        SCIP_Real lambda = SCIPgetDualsolLinear(scip, _num_partitions_cons);
        if (SCIPgetSolOrigObj(scip_pricer, sol) < lambda)
        {
            SCIP_CALL(addPartitionVar(scip, sol));
        }
    }
    
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::addRemainingConnectivityCons(Graph::vertex_descriptor t)
{
    for (auto c : connectivity_cons)
    {
        SCIP_CALL(SCIPdelCons(scip_pricer, c));
        SCIP_CALL(SCIPreleaseCons(scip_pricer, &c));
    }
    connectivity_cons.clear();
    
    for (auto s : _T)
    {
        SCIP_CONS* cons1;
        if (s == t)
        {
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1, "first", 0, NULL, NULL, -1.0, -1.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        }
        else
        {
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1, "first", 0, NULL, NULL, 0.0, 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        }
        
        for (auto q = out_edges(s, *g); q.first != q.second; ++q.first)
        {
            auto source = boost::source(*q.first, *g);
            auto target = boost::target(*q.first, *g);
            if (source == s)
            {
                SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, e1[*q.first], 1.0));
                SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, e2[*q.first], -1.0));
            }
            else if (target == s)
            {
                SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, e2[*q.first], 1.0));
                SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, e1[*q.first], -1.0));
            }
            else
            {
                std::cerr << "häääää?" << std::endl;
                exit(1);
            }
        }
        
        if (s == t)
        {
            for (auto q = vertices(*g); q.first != q.second; ++q.first)
            {
                SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, x[*q.first], -1.0));
            }
        }
        else
        {
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, x[s], 1.0));
        }
        SCIP_CALL(SCIPaddCons(scip_pricer, cons1));
        connectivity_cons.push_back(cons1);
    }
    
    return SCIP_OKAY;
}

SCIP_RETCODE ObjPricerLinFit::addPartitionVar(SCIP* scip, SCIP_SOL* sol)
{
    SCIP_Real gamma_P = 0.0;
    for(auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        gamma_P += SCIPgetSolVal(scip_pricer, sol, delta[*s.first]);
    }
    
    SCIP_VAR* x_P;
    SCIP_CALL(SCIPcreateVar(scip, & x_P, "x_P", 0.0, 1.0, gamma_P, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddPricedVar(scip, x_P, 1.0));
    
    // add coefficients to constraints (of the master problem)
    for (auto s = vertices(*g); s.first != s.second; ++s.first)
    {
        if (SCIPgetSolVal(scip_pricer, sol, x[*s.first]) == 1)
        {
            SCIP_CALL(SCIPaddCoefLinear(scip, _partitioning_cons[*s.first], x_P, 1.0));
        }
    }
    SCIP_CALL(SCIPaddCoefLinear(scip, _num_partitions_cons, x_P, 1.0));
    
    return SCIP_OKAY;
}
