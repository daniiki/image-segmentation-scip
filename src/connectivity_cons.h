#include <objscip/objscip.h>

using namespace scip;

class ConnectivityCons : public ObjConshdlr 
{
    ConnectivityCons();
    
    SCIP_DECL_CONSTRANS (scip_trans);
    
    SCIP_DECL_CONSENFOLP (scip_enfolp);
    
    SCIP_DECL_CONSENFOPS (scip_enfops);
    
    SCIP_DECL_CONSCHECK (scip_check);
    
    SCIP_DECL_CONSLOCK (scip_lock);
};


