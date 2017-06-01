#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>

#include <scip/scipdefplugins.h>

#include <iostream>
#include <algorithm>

struct Superpixel
{
    unsigned int color;
};
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, Superpixel> Graph;

Graph create_random_graph()
{
    boost::minstd_rand gen; // random number generator
    Graph g;
    generate_random_graph(g, 6, 10, gen,
                          false, // no parallel edges
                          false // no self-edges
                         );
    
    // associate random color with superpixels
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        g[*p.first].color = gen() % 256;
    }
    return g;
}

int bigM(Graph g)
{
    int maxcolor = 0;
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (g[*p.first].color > maxcolor)
        {
            maxcolor = g[*p.first].color;
        }
    }
}

int pricing_problem(Graph g, int k, std::vector<double> mu, std::vector<Graph::vertex_descriptor> T)
{
    int M = bigM(g);
    int n = num_vertices(g);
    
    SCIP* scip;
    SCIP_CALL(SCIPcreate(& scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    
    //create pricing problem
    SCIP_CALL(SCIPcreateProb(scip, "pricing_problem", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE));
    
    //variables
    std::vector<SCIP_VAR*> x;
    std::vector<SCIP_VAR*> delta;
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        SCIP_VAR* x_s;
        SCIP_CALL(SCIPcreateVar(scip, & x_s, "x_s", 0.0, 1.0, -mu[*p.first], SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, x_s));
        x.push_back(x_s);
        
        SCIP_VAR* delta_s;
        SCIP_CALL(SCIPcreateVar(scip, & delta_s, "delta_s", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, delta_s));
        delta.push_back(delta_s);
    }
    SCIP_VAR* c_P;
    SCIP_CALL(SCIPcreateVar(scip, & c_P, "c_P", -SCIPinfinity(scip), SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddVar(scip, c_P));
    
    // float variables for connectivity constraints
    std::map<Graph::edge_descriptor, SCIP_VAR*> e1; // e_source,target
    std::map<Graph::edge_descriptor, SCIP_VAR*> e2; // e_target,source
    for (auto p = edges(g); p.first != p.second; ++p.first)
    {
        SCIP_VAR* e_vw;
        SCIP_CALL(SCIPcreateVar(scip, & e_vw, "e_vw", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, e_vw));
        e1[*p.first] = e_vw;
        
        SCIP_VAR* e_wv;
        SCIP_CALL(SCIPcreateVar(scip, & e_wv, "e_wv", 0.0, SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, e_wv));
        e2[*p.first] = e_wv;
    }

    //constraints
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        SCIP_CONS* cons1;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons1, "first", 0, NULL, NULL, -SCIPinfinity(scip), M + g[*p.first].color, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons1, x[*p.first], M));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons1, delta[*p.first], -1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons1, c_P, 1.0));
        SCIP_CALL(SCIPaddCons(scip, cons1));
        
        SCIP_CONS* cons2;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip), M - g[*p.first].color, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, x[*p.first], M));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, delta[*p.first], -1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, c_P, -1.0));
        SCIP_CALL(SCIPaddCons(scip, cons2));
        
        SCIP_CONS* cons3;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip), 0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons3, x[*p.first], -M));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons3, delta[*p.first], 1.0));
        SCIP_CALL(SCIPaddCons(scip, cons3));
    }
    
    // connectivity constraints
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (std::find(T.begin(), T.end(), *p.first) != T.end())
        {
            SCIP_CONS* cons1;
            SCIP_CALL(SCIPcreateConsLinear(scip, & cons1, "first", 0, NULL, NULL, 0.0, 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            
            for (auto q = out_edges(*p.first, g); q.first != q.second; ++q.first)
            {
                auto source = boost::source(*q.first, g);
                auto target = boost::target(*q.first, g);
                if (source == *p.first)
                {
                    SCIP_CALL(SCIPaddCoefLinear(scip, cons1, e1[*q.first], 1.0));
                    SCIP_CALL(SCIPaddCoefLinear(scip, cons1, e2[*q.first], -1.0));
                }
                else if (target == *p.first)
                {
                    SCIP_CALL(SCIPaddCoefLinear(scip, cons1, e2[*q.first], 1.0));
                    SCIP_CALL(SCIPaddCoefLinear(scip, cons1, e1[*q.first], -1.0));
                }
                else
                    std::cerr << "häääää?" << std::endl;
            }
            
            SCIP_CALL(SCIPaddCoefLinear(scip, cons1, x[*p.first], 1.0));
        }
    }
    
    for ( auto p = edges(g); p.first != p.second; ++p.first)
    {
        auto source = boost::source(*p.first, g);
        auto target = boost::target(*p.first, g);
        
        SCIP_CONS* cons2;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons2, "second", 0, NULL, NULL, -SCIPinfinity(scip), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, e1[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, e2[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, x[source], k - n));
        
        SCIP_CONS* cons3;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons3, "third", 0, NULL, NULL, -SCIPinfinity(scip), 0.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons3, e1[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons3, e2[*p.first], 1.0));
        SCIP_CALL(SCIPaddCoefLinear(scip, cons3, x[target], k - n));
    }
    
    //TODO add constraints for each t in T
}
