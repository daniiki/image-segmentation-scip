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

    virtual SCIP_DECL_CONSTRANS(scip_trans);

    virtual SCIP_DECL_CONSENFOLP(scip_enfolp);

    virtual SCIP_DECL_CONSENFOPS(scip_enfops);

    virtual SCIP_DECL_CONSCHECK(scip_check);
 
    virtual SCIP_DECL_CONSLOCK(scip_lock);

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

SCIP_RETCODE SCIPcreateConsConnectivity(
    SCIP* scip,
    SCIP_CONS** cons,
    const char* name,
    SCIP_Bool             initial,            /**< should the LP relaxation of constraint be in the initial LP? */
    SCIP_Bool             separate,           /**< should the constraint be separated during LP processing? */
    SCIP_Bool             enforce,            /**< should the constraint be enforced during node processing? */
    SCIP_Bool             check,              /**< should the constraint be checked for feasibility? */
    SCIP_Bool             propagate,          /**< should the constraint be propagated during node processing? */
    SCIP_Bool             local,              /**< is constraint only valid locally? */
    SCIP_Bool             modifiable,         /**< is constraint modifiable (subject to column generation)? */
    SCIP_Bool             dynamic,            /**< is constraint dynamic? */
    SCIP_Bool             removable           /**< should the constraint be removed from the LP due to aging or cleanup? */
);
