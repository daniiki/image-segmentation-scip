#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

/**
 * Class representing pricing problem
 * After each iteration of the master problem, the `scip_redcost` method is called.
 * This method should generate a new column with negative reduced costs and add it to the problem.
 */
class SegmentPricer : public ObjPricer
{
public:
    /**
     * Constructor for the pricer class
     */
    SegmentPricer(
        SCIP* scip, ///< master SCIP instance
        Graph& g_, ///< the graph of superpixels
        std::vector<Graph::vertex_descriptor> master_nodes, ///< master nodes of all segments 
        std::vector<SCIP_CONS*> partitioning_cons, 
        SCIP_CONS* num_segments_cons
        );

    /**
     * Set up pricer
     * This replaces variables and constraints by their counterparts in the transformed problem.
     */
    SCIP_DECL_PRICERINIT(scip_init); 

    /**
     * Add variables \f$x_s\f$ for each superpixel \f$s\in\mathcal{S}\f$ to the pricing problem represented by `scip_pricer`
     */
    SCIP_RETCODE setupVars(SCIP* scip_pricer, Graph::vertex_descriptor t);

    /**
     * Reduced cost pricing method of variable pricer for feasible LPs
     * Solves the pricing problem for each master node \f$t\in T\f$.
     * If the reduced costs are negative, i.e. 
     * \f[-\sum_{s\in\mathcal{S}} x_s\cdot\mu_s + \sum_{s\in\mathcal{S}} x_s\cdot|y_t-y_s| < \lambda,\f]
     * the generated segment consisting of all superpixels \f$s\f$ for which \f$x_s = 1\f$ is added to the master problem.
     * @todo add a heuristic
     */
    virtual SCIP_DECL_PRICERREDCOST(scip_redcost);
    
    std::pair<SCIP_Real, std::vector<Graph::vertex_descriptor>> heuristic(
        SCIP* scip,
        Graph::vertex_descriptor master_node,
        SCIP_Real lambda
        );

    /**
     * Calls `addPartitionVar` with the vector of all superpixels \f$s\f$ for which \f$x_s = 1\f$
     */
    SCIP_RETCODE addPartitionVarFromPricerSCIP(SCIP* scip, SCIP* scip_pricer, SCIP_SOL* sol, Graph::vertex_descriptor t);
    
    /**
     * Adds a new segment variable to the master problem
     * This also adds the variable to the appropriate existing constraints.
     */
    SCIP_RETCODE addPartitionVar(SCIP* scip, Graph::vertex_descriptor master_node, std::vector<Graph::vertex_descriptor> superpixels);

private:
    Graph& g;
    std::vector<Graph::vertex_descriptor> master_nodes;
    std::vector<SCIP_CONS*> partitioning_cons;
    SCIP_CONS* num_segments_cons;

    // pricing problem data
    std::vector<SCIP*> scip_pricers; // k instances (one for each t in T)
    int _bigM;
    int _n;

    /** 
     * Problem data class for the pricing problem
     */
    class PricerData : public ObjProbData
    {
    public:
        PricerData() : ObjProbData()
        {}

        std::vector<SCIP_VAR*> x; ///< variables \f$x_s\f$ for each superpixel \f$s\in\mathcal{S}\f$
    };
};
