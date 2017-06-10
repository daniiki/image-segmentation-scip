#include <objscip/objscip.h>
#include <boost/graph/adjacency_list.hpp>

class ObjPricerLinFit : public ObjPricer
{
public:
    ObjPricerLinFit(SCIP* scip, Graph* g, int k, std::vector<Graph::vertex_descriptor> T, std::vector<SCIP_CONS*> partitioning_cons, SCIP_CONS* num_partitions_cons);
    
    SCIP_RETCODE initialSetup(); // set up scip_pricer
    
    SCIP_RETCODE setupVars();
    
    SCIP_RETCODE setupCons();
    
    SCIP_RETCODE setupConnectivityCons();
    
    virtual SCIP_DECL_PRICERREDCOST(scip_redcost);
    
    SCIP_RETCODE 
    
    SCIP_RETCODE addPartitionVar(SCIP_SOL* sol);
    
private:
    Graph* _g;
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
    std::vector<SCIP_VAR*> delta;
};
