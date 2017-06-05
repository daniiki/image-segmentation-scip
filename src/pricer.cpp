#include <objscip/objscip.h>
#include <boost/graph/adjacency_list.hpp>

class ObjPricerLinFit : public ObjPricer
{
public:
    ObjPricerLinFit(SCIP* scip, Graph* g, int k, std::vector<Graph::vertex_descriptor> T, std::vector<SCIP_VAR*> x, std::vector<SCIP_CONS*> partitioning_cons, SCIP_CONS* num_partitions_cons) :
        ObjPricer(scip, "name", "description", 0, TRUE),
        _g(g), _k(k), _T(T), _x(x), _partitioning_cons(partitioning_cons), _num_partitions_cons(num_partitions_cons)
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
    }
    
    virtual SCIP_DECL_PRICERREDCOST(scip_redcost)
    {
        SCIP* scip_pricer;
        SCIP_CALL(SCIPcreate(& scip_pricer));
        SCIP_CALL(SCIPincludeDefaultPlugins(scip_pricer));
        
        //create pricing problem
        SCIP_CALL(SCIPcreateProb(scip_pricer, "pricing_problem", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPsetObjsense(scip_pricer, SCIP_OBJSENSE_MINIMIZE));
        
        //variables
        std::vector<SCIP_VAR*> x;
        std::vector<SCIP_VAR*> delta;
        for (auto p = vertices(g); p.first != p.second; ++p.first)
        {
            SCIP_Real mu_s = SCIPgetDualsolLinear(scip, _partitioning_cons[*p.first]);
            SCIP_VAR* x_s;
            SCIP_CALL(SCIPcreateVar(scip_pricer, & x_s, "x_s", 0.0, 1.0, -mu_s, SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
            SCIP_CALL(SCIPaddVar(scip_pricer, x_s));
            x.push_back(x_s);
            
            SCIP_VAR* delta_s;
            SCIP_CALL(SCIPcreateVar(scip_pricer, & delta_s, "delta_s", 0.0, SCIPinfinity(scip_pricer), 1.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
            SCIP_CALL(SCIPaddVar(scip_pricer, delta_s));
            delta.push_back(delta_s);
        }
        SCIP_VAR* c_P;
        SCIP_CALL(SCIPcreateVar(scip_pricer, & c_P, "c_P", -SCIPinfinity(scip_pricer), SCIPinfinity(scip_pricer), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip_pricer, c_P));
        
        // float variables for connectivity constraints
        std::map<Graph::edge_descriptor, SCIP_VAR*> e1; // e_source,target
        std::map<Graph::edge_descriptor, SCIP_VAR*> e2; // e_target,source
        for (auto p = edges(g); p.first != p.second; ++p.first)
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

        //constraints
        for (auto p = vertices(g); p.first != p.second; ++p.first)
        {
            SCIP_CONS* cons1;
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1, "first", 0, NULL, NULL, -SCIPinfinity(scip_pricer), M + g[*p.first].color, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, x[*p.first], M));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, delta[*p.first], -1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1, c_P, 1.0));
            SCIP_CALL(SCIPaddCons(scip_pricer, cons1));
            
            SCIP_CONS* cons2;
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip_pricer), M - g[*p.first].color, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, x[*p.first], M));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, delta[*p.first], -1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, c_P, -1.0));
            SCIP_CALL(SCIPaddCons(scip_pricer, cons2));
            
            SCIP_CONS* cons3;
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, x[*p.first], -M));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, delta[*p.first], 1.0));
            SCIP_CALL(SCIPaddCons(scip_pricer, cons3));
        }
        
        // connectivity constraints
        for (auto p = edges(g); p.first != p.second; ++p.first)
        {
            auto source = boost::source(*p.first, g);
            auto target = boost::target(*p.first, g);
            
            SCIP_CONS* cons2;
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, e1[*p.first], 1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, e2[*p.first], 1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons2, x[source], k - n));
            SCIP_CALL(SCIPaddCons(scip_pricer, cons2));
            
            SCIP_CONS* cons3;
            SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip_pricer), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, e1[*p.first], 1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, e2[*p.first], 1.0));
            SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons3, x[target], k - n));
            SCIP_CALL(SCIPaddCons(scip_pricer, cons3));
        }
        
        // constraints for each t in T
        std::vector<SCIP_CONS*> cons1;
        for (auto t : T)
        {
            for (auto c : cons1)
            {
                SCIP_CALL(SCIPdelCons(scip_pricer, c));
                SCIP_CALL(SCIPreleaseCons(scip_pricer, c);
            }
            cons1.clear();
            
            for (auto p = vertices(g); p.first != p.second; ++p.first)
            {
                SCIP_CONS* cons1_s;
                if (*p.first == t)
                {
                    SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1_s, "first", 0, NULL, NULL, -1.0, -1.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
                }
                else
                {
                    SCIP_CALL(SCIPcreateConsLinear(scip_pricer, & cons1_s, "first", 0, NULL, NULL, 0.0, 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
                }
                
                for (auto q = out_edges(*p.first, g); q.first != q.second; ++q.first)
                {
                    auto source = boost::source(*q.first, g);
                    auto target = boost::target(*q.first, g);
                    if (source == *p.first)
                    {
                        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, e1[*q.first], 1.0));
                        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, e2[*q.first], -1.0));
                    }
                    else if (target == *p.first)
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
                
                if (*p.first == t)
                {
                    for (auto q = vertices(g); q.first != q.second; ++q.first)
                    {
                        SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, x[*q.first], -1.0));
                    }
                }
                else
                {
                    SCIP_CALL(SCIPaddCoefLinear(scip_pricer, cons1_s, x[*p.first], 1.0));
                }
                SCIP_CALL(SCIPaddCons(scip_pricer, cons1_s));
                cons1.push_back(cons1_s);
            }
            SCIP_CALL(SCIPsolve(scip_pricer));
            SCIP_SOL* sol = SCIPgetBestSol(scip_pricer);
            SCIP_Real lambda = SCIPgetDualsolLinear(scip, _num_partitions_cons);
            if (SCIPgetSolOrigObj(scip_pricer, sol) < lambda)
            {
                SCIP_VAR* x_P;
                SCIP_CALL(SCIPcreateVar(scip, & x_P, "x_P", 0.0, 1.0, gamma_P, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
                SCIP_CALL(SCIPaddVar(scip, x_P));
                //to do: iwas mit delta
                
            }    
		

        }
    }
    
private: 
    Graph* _g;
    int _k;
    std::vector<Graph::vertex_descriptor> _T;
    std::vector<SCIP_VAR*> _x;
    std::vector<SCIP_CONS*> _partitioning_cons;
    std::vector<SCIP_CONS*> _num_partitions_cons;
    int _bigM;
    int _n;
};
