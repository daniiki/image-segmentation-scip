#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>

#include <scip/scipdefplugins.h>

#include <iostream>
#include <math.h>

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

int master_problem(Graph g, int k, std::vector<std::set<Graph::vertex_descriptor>> partitions)
{
    SCIP* scip;
    SCIP_CALL(SCIPcreate(& scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    
    //create master problem
    SCIP_CALL(SCIPcreateProb(scip, "master_problem", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE));
    
    std::vector<SCIP_VAR*> vars;
    // vars[i] belongs to partition partitions[i]
    for (auto partition : partitions)
    {
        SCIP_VAR* var;
        SCIP_CALL(SCIPcreateVar(scip, & var, "x_P", 0.0, 1.0, gamma(g, partition), SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, var));
        vars.push_back(var);
    }
    
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
            SCIP_CONS* cons1;
            SCIP_CALL(SCIPcreateConsLinear(scip, & cons1, "first", 0, NULL, NULL, 1.0, 1.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
            for (int i = 0; i != partitions.size(); ++i)
            {
                if (partitions[i].find(*p.first) != partitions[i].end())
                {
                    SCIP_CALL(SCIPaddCoefLinear(scip, cons1, vars[i], 1.0));
                }
            }
            SCIP_CALL(SCIPaddCons(scip, cons1));
    }
    
    SCIP_CONS* cons2;
    SCIP_CALL(SCIPcreateConsLinear(scip, & cons2, "first", 0, NULL, NULL, k, k, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
    for (int i = 0; i != partitions.size(); ++i)
    {
        SCIP_CALL(SCIPaddCoefLinear(scip, cons2, vars[i], 1.0));
    }
    SCIP_CALL(SCIPaddCons(scip, cons2));
    
    SCIP_CALL(SCIPsolve(scip));
    SCIP_SOL* sol = SCIPgetBestSol(scip);
    
    return 0;
}


template <typename T>
auto subsets(std::set<T> set)
{
    if (set.size() == 1)
    {
        std::set<T> emptyset = {};
        std::set<T> singleton = { *set.begin() };
        return std::vector<std::set<T>> { emptyset, singleton };
    }
    else
    {
        auto elem = *set.begin();
        set.erase(set.begin());
        auto sets1 = subsets(set);
        auto sets2 = sets1;
        for(auto it = sets2.begin(); it != sets2.end(); ++it)
        {
            it->insert(elem);
        }
        sets1.insert(sets1.end(), sets2.begin(), sets2.end());
        return sets1;
    }
}

int main()
{
    auto g = create_random_graph();
    std::set<Graph::vertex_descriptor> superpixels;
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        superpixels.insert(*p.first);
    }
    
    master_problem(g, 2, subsets(superpixels));
    return 0;
}
