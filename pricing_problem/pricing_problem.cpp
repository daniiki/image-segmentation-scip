#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>

#include <scip/scipdefplugins.h>

#include <iostream>


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
            maxcolor = g[*p.first].color;ß
        }
    }
}

int pricing_problem(Graph g, int k, std::vector<double> mu)
{
    int M = bigM(g);
    
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
        SCIP_CALL(SCIPcreateVar(scip, & x_s, "x_s", 0.0, 1.0, -mu[*p.first], SCIP_VARTYPE_BINARY, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, x_s));
        x.push_back(x_s);
        
        SCIP_VAR* delta_s;
        SCIP_CALL(SCIPcreateVar(scip, & delta_s, "delta_s", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, delta_s));
        delta.push_back(delta_s);
    }
    SCIP_VAR* c_P;
    SCIP_CALL(SCIPcreateVar(scip, & c_P, "c_P", -SCIPinfinity(scip), SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONtINUOUS, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddVar(scip, c_P))

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
    }
    
    //TODO connectivity constraints
}
