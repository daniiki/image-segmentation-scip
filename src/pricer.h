#include <objscip/objscip.h>
#include "graph.h"

using namespace scip;

class ObjPricerLinFit : public ObjPricer
{
public:
    ObjPricerLinFit(SCIP* scip, Graph* g_, int k, std::vector<Graph::vertex_descriptor> T, std::vector<SCIP_CONS*> partitioning_cons, SCIP_CONS* num_partitions_cons);
    
    SCIP_DECL_PRICERINIT(scip_init); // set up scip_pricer
    
    SCIP_RETCODE setupVars();
    
    SCIP_RETCODE setupCons();
    
    SCIP_RETCODE setupConnectivityCons();
    
    virtual SCIP_DECL_PRICERREDCOST(scip_redcost);
    
    SCIP_RETCODE addRemainingConnectivityCons(Graph::vertex_descriptor t);
    
    SCIP_RETCODE addPartitionVar(SCIP* scip, SCIP_SOL* sol);
    
private:
    Graph* g;
    int _k;
    std::vector<Graph::vertex_descriptor> _T;
    std::vector<SCIP_CONS*> _partitioning_cons;
    SCIP_CONS* _num_partitions_cons;
    
    // pricing problem data
    SCIP* scip_pricer;
    int _bigM;
    int _n;
    
    // pricing problem variables
    std::vector<SCIP_VAR*> x;
    SCIP_VAR* c_P;
    std::vector<SCIP_VAR*> delta;
    std::map<Graph::edge_descriptor, SCIP_VAR*> e1; // e_source,target
    std::map<Graph::edge_descriptor, SCIP_VAR*> e2; // e_target,source
    
    // pricing problem constraints
    std::vector<SCIP_CONS*> connectivity_cons;
};
