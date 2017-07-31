#ifndef GRAPH_H
#define GRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/subgraph.hpp>
#include <scip/scip.h>

using namespace boost;

struct Superpixel
{
    SCIP_Real color;
};

// setS disallows parallel edges
typedef subgraph<adjacency_list<
        setS, vecS, undirectedS,
        Superpixel,
        property<edge_index_t, size_t>
    >>
    Graph;
 
#endif
