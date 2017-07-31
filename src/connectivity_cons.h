#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

class ConnectivityCons : public ObjConshdlr 
{
public:
    ConnectivityCons(
        SCIP* scip,
        Graph& g_,
        std::vector<Graph::vertex_descriptor>& T_,
        Graph::vertex_descriptor t_,
        std::vector<SCIP_VAR*>& x_
        );

    SCIP_DECL_CONSENFOLP(scip_enfolp);

    SCIP_DECL_CONSENFOPS(scip_enfops);

    SCIP_DECL_CONSCHECK(scip_check);
 
    SCIP_DECL_CONSLOCK(scip_lock);

private:
    size_t findComponents(
        SCIP* scip,
        SCIP_SOL* sol,
        Graph& subgraph, // subgraph with all superpixels for which x_s is 1
        std::vector<int>& component
);

    Graph& g;
    std::vector<Graph::vertex_descriptor>& T;
    Graph::vertex_descriptor t;
    std::vector<SCIP_VAR*>& x;
};
