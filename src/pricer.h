#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

class ObjPricerLinFit : public ObjPricer
{
public:
    ObjPricerLinFit(SCIP* scip, Graph* g_, int k, std::vector<Graph::vertex_descriptor> T, std::vector<SCIP_CONS*> partitioning_cons, SCIP_CONS* num_partitions_cons);

    SCIP_DECL_PRICERINIT(scip_init); // set up scip_pricer

    SCIP_RETCODE setupVars(SCIP* scip_pricer, Graph::vertex_descriptor t);

    virtual SCIP_DECL_PRICERREDCOST(scip_redcost);
    
    SCIP_Real heuristic(SCIP* scip, Graph::vertex_descriptor t, std::vector<Graph::vertex_descriptor>& partition, SCIP_Real& fitting_error);

    SCIP_RETCODE addPartitionVarFromPricerSCIP(SCIP* scip, SCIP* scip_pricer, SCIP_SOL* sol, Graph::vertex_descriptor t);
    
    SCIP_RETCODE addPartitionVar(SCIP* scip, std::vector<Graph::vertex_descriptor> superpixels, SCIP_Real gamma_P);

private:
    Graph* g;
    int _k;
    std::vector<Graph::vertex_descriptor> _T;
    std::vector<SCIP_CONS*> _partitioning_cons;
    SCIP_CONS* _num_partitions_cons;

    // pricing problem data
    std::vector<SCIP*> scip_pricers; // k instances (one for each t in T)
    int _bigM;
    int _n;

    // problem data class for the pricing problem
    class PricerData : public ObjProbData
    {
    public:
        PricerData() : ObjProbData()
        {}

        // pricing problem variables
        std::vector<SCIP_VAR*> x;
        std::vector<std::vector<SCIP_VAR*>> e; // e[i][j] is e_i,j
    };
};
