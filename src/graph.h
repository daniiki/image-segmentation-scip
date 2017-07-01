#ifndef GRAPH_H
#define GRAPH_H

#include <boost/graph/adjacency_list.hpp>

struct Superpixel
{
    unsigned int color;
};

// setS disallows parallel edges
typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS, Superpixel> Graph;
 
#endif
