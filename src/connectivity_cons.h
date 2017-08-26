#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

/**
 * Class for representing connectivity constraints
 * This class uses cutting planes to make disconnected segments infeasible.
 * Whenever one of the methods `scip_enfolp`, `scip_enfops`, `scip_check` is called,
 * the connected components of the subgraph containing all superpixels \f$s\in\mathcal{S}\f$
 * for which \f$x_s = 1\f$ are calculated.
 * In `scip_enfolp`, a cutting plane is added if the current solution is infeasible.
 */
class ConnectivityCons : public ObjConshdlr 
{
public:
    /**
     * Constructor for the connectivity constraints class
     */
    ConnectivityCons(
        SCIP* scip, ///< pricer SCIP instance
        Graph& g_, ///< the graph of superpixels
        std::vector<Graph::vertex_descriptor>& master_nodes_, ///< master nodes of all segements 
        Graph::vertex_descriptor master_node_, ///< master node of the current segement
        std::vector<SCIP_VAR*>& superpixel_vars_ ///< vector of variables \f$x_s\f$ for each superpixel \f$s\in\mathcal{S}\f$
        );

    /**
     * Transforms constraint data into data belonging to the transformed problem
     */
    virtual SCIP_DECL_CONSTRANS(scip_trans);
    
    /**
     * Separation method of constraint handler for LP solution
     */
    SCIP_DECL_CONSSEPALP(scip_sepalp);

    /**
     * Separation method of constraint handler for arbitrary primal solution
     */
    SCIP_DECL_CONSSEPASOL(scip_sepasol);

    /**
     * Constraint enforcing method of constraint handler for LP solutions
     */
    virtual SCIP_DECL_CONSENFOLP(scip_enfolp);

    /**
     * Constraint enforcing method of constraint handler for pseudo solutions
     */
    virtual SCIP_DECL_CONSENFOPS(scip_enfops);

    /**
     * Feasibility check method of constraint handler for primal solutions
     */
    virtual SCIP_DECL_CONSCHECK(scip_check);
 
    /**
     * Variable rounding lock method of constraint handler
     */ 
    virtual SCIP_DECL_CONSLOCK(scip_lock);

private:
    /**
     * Finds all connected components in a subgraph
     * @return the number of connected components
     */
    size_t findComponents(
        SCIP* scip, ///< pricer SCIP instance
        SCIP_SOL* sol, ///< current primal solution or NULL
        Graph& subgraph, ///< subgraph with all superpixels for which \f$x_s = 1\f$
        std::vector<int>& component ///< `component[s]` will be the index of the connected component the superpixel s belongs to
    );

    /**
     * Adds cutting plane, if possible.
     * If the current solution is infeasible, a cutting plane of the following form is added
     * for every superpixel \f$s\f$ in a component \f$C\f$ that is not connected to the master node \f$t\f$:
     * \f[\sum_{s'\in\delta(C)}x_{s'} \geq x_s\f]
     */
    SCIP_RETCODE sepaConnectivity(
        SCIP* scip,
        SCIP_CONSHDLR* conshdlr,
        SCIP_SOL* sol,
        SCIP_RESULT* result
    );

    Graph& g;
    std::vector<Graph::vertex_descriptor>& master_nodes;
    Graph::vertex_descriptor master_node;
    std::vector<SCIP_VAR*>& superpixel_vars;
};

/**
 * Creates and captures connectivity constraint
 */
SCIP_RETCODE SCIPcreateConsConnectivity(
    SCIP* scip, ///< pricer SCIP instance
    SCIP_CONS** cons, ///< pointer to hold the created constraint
    const char* name, ///< name of constraint
    SCIP_Bool initial, ///< should the LP relaxation of constraint be in the initial LP? */
    SCIP_Bool separate, ///< should the constraint be separated during LP processing? */
    SCIP_Bool enforce, ///< should the constraint be enforced during node processing? */
    SCIP_Bool check, ///< should the constraint be checked for feasibility? */
    SCIP_Bool propagate, ///< should the constraint be propagated during node processing? */
    SCIP_Bool local, ///< is constraint only valid locally? */
    SCIP_Bool modifiable, ///< is constraint modifiable (subject to column generation)? */
    SCIP_Bool dynamic, ///< is constraint dynamic? */
    SCIP_Bool removable ///< should the constraint be removed from the LP due to aging or cleanup? */
);
