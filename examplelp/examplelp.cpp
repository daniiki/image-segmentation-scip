#include <iostream>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>

int main()
{
    SCIP* scip;
    SCIP_CALL(SCIPcreate(& scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));

    // Creating the problem
    SCIP_CALL(SCIPcreateProb(scip, "examplelp", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MAXIMIZE));

    // Creating the variables
    SCIP_VAR* x;
    SCIP_CALL(SCIPcreateVar(scip, & x, "x", 0.0, SCIPinfinity(scip), 6.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddVar(scip, x));

    SCIP_VAR* y;
    SCIP_CALL(SCIPcreateVar(scip, & y, "y", 0.0, SCIPinfinity(scip), 3.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddVar(scip, y));

    SCIP_VAR* z;
    SCIP_CALL(SCIPcreateVar(scip, & z, "z", 0.0, SCIPinfinity(scip), 4.0, SCIP_VARTYPE_CONTINUOUS, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPaddVar(scip, z));
    
    // Creating constraints
    SCIP_CONS* fst;
    SCIP_CALL(SCIPcreateConsLinear(scip, & fst, "first", 0, NULL, NULL, 10.0, 10.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
    SCIP_CALL(SCIPaddCoefLinear(scip, fst, x, 5.0));
    SCIP_CALL(SCIPaddCoefLinear(scip, fst, y, -1.0));
    SCIP_CALL(SCIPaddCons(scip, fst));

    SCIP_CONS* snd;
    SCIP_CALL(SCIPcreateConsLinear(scip, & snd, "second", 0, NULL, NULL, -SCIPinfinity(scip), 10.0, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE));
    SCIP_CALL(SCIPaddCoefLinear(scip, snd, y, 1.0));
    SCIP_CALL(SCIPaddCoefLinear(scip, snd, z, 1.0));
    SCIP_CALL(SCIPaddCons(scip, snd));

    // Solving the problem
    SCIP_CALL(SCIPsolve(scip));
    SCIP_SOL* sol = SCIPgetBestSol(scip);
    std::cout << "x: " << SCIPgetSolVal(scip, sol, x) << std::endl;
    std::cout << "y: " << SCIPgetSolVal(scip, sol, y) << std::endl;
    std::cout << "z: " << SCIPgetSolVal(scip, sol, z) << std::endl;
}
