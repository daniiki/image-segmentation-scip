#ifndef GRAPH_H
#define GRAPH_H

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/subgraph.hpp>
#include <scip/scip.h>

using namespace boost;

//typename Graph;

struct Pixel
{
    uint32_t x;
    uint32_t y;

    Pixel(uint32_t x_, uint32_t y_) : x(x_), y(y_)
    {}
};

struct Superpixel
{
    SCIP_Real color;
    std::vector<Pixel> pixels;
};

// setS disallows parallel edges
typedef subgraph<adjacency_list<
        setS, vecS, undirectedS,
        Superpixel,
        property<edge_index_t, size_t,
        property<edge_weight_t, size_t>> // edge weight is the number of neighbouring pixels,
                                         // i.e. the number of edges in the pixel graph
                                         // connecting both superpixels
    >>
    Graph;
 
#endif
