#ifndef VARDATA_H
#define VARDATA_H

#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

class ObjVardataSegment : public ObjVardata
{
public:
    ObjVardataSegment(std::vector<Graph::vertex_descriptor> superpixels) :
        ObjVardata(),
        _superpixels(superpixels)
    {}
    
    bool containsSuperpixel(Graph::vertex_descriptor superpixel)
    {
        return std::find(_superpixels.begin(), _superpixels.end(), superpixel) != _superpixels.end();
    }
    
    std::vector<Graph::vertex_descriptor> getSuperpixels()
    {
        return _superpixels;
    }
    
private:
    std::vector<Graph::vertex_descriptor> _superpixels;
};

#endif
