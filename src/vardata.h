#ifndef VARDATA_H
#define VARDATA_H

#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

/**
 * Variable data associated with segment variables \f$x_P\f$ 
 */
class ObjVardataSegment : public ObjVardata
{
public:
    ObjVardataSegment(
        std::vector<Graph::vertex_descriptor> superpixels_ ///< superpixels contained in the segment \f$P\f$
        ) :
        ObjVardata(),
        superpixels(superpixels_)
    {}
    
    bool containsSuperpixel(Graph::vertex_descriptor superpixel)
    {
        return std::find(superpixels.begin(), superpixels.end(), superpixel) != superpixels.end();
    }
    
    std::vector<Graph::vertex_descriptor> getSuperpixels()
    {
        return superpixels;
    }
    
private:
    std::vector<Graph::vertex_descriptor> superpixels;
};

#endif
