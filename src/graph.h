#ifndef GRAPH_H
#define GRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <scip/scip.h>

struct Superpixel
{
    SCIP_Real color;
};

// setS disallows parallel edges
typedef boost::adjacency_list<boost::setS, boost::vecS, boost::undirectedS, Superpixel> Graph;
 
#endif
