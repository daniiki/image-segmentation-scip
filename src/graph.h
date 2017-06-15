#ifndef GRAPH_H
#define GRAPH_H

#include <boost/graph/adjacency_list.hpp>

struct Superpixel
{
    unsigned int color;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, Superpixel> Graph;
 
#endif
