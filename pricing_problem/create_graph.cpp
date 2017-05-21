#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/erdos_renyi_generator.hpp>
#include <boost/random/linear_congruential.hpp>

#include <boost/graph/random.hpp>

#include <iostream>

typedef boost::adjacency_list<> Graph;
typedef boost::erdos_renyi_iterator<boost::minstd_rand, Graph> ERGen;

int main()
{
    boost::minstd_rand gen; // random number generator
    // Create graph with 100 nodes and edges with probability 0.05
    Graph g(ERGen(gen, 100, 0.05), ERGen(), 100);
    std::cout << num_vertices(g) << std::endl;
    
    boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> g1;
    generate_random_graph(g1, 50, 30, gen,
                          false, // no parallel edges
                          false // no self-edges
                         );
    std::cout << num_vertices(g1) << std::endl;
    
    return 0;
}
