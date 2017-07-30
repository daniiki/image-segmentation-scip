#include "connectivity_cons.h"

size_t ConnectivityCons::findComponents(
    SCIP* scip,
    Graph& subgraph // subgraph with all superpixels for which x_s is 1
)
{
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (SCIPisEQ(SCIPgetVarSol(scip, x[*p.first]), 1.0))
        {
            add_vertex(*p.first, subgraph);
        }
    }
    std::vector<int> component(num_vertices(g));
    size_t num_components = connected_components(subgraph, &component[0]);
    return num_components;
}

SCIP_DECL_CONSTRANS(ConnectivityCons::scip_trans)
{
    
}

SCIP_DECL_CONSENFOLP(ConnectivityCons::scip_enfolp)
{
    Graph& subgraph = g.create_subgraph();
    size_t num_components = findComponents(scip, 
    if (num_components == 1)
    {
        *result = SCIP_FEASIBLE;
    }
    else
    {
        for (int i = 0; i < num_components; i++)
        {
            if (i == component[t])
            {
                continue;
            }
            auto p = vertices(subgraph);
            for (; p.first != p.second; ++p.first)
            {
                if (component[*p.first] == i)
                {
                    break;
                }
            }
            SCIP_ROW* row;
            SCIP_CALL(SCIPcreateEmptyRowCons(scip, &row, conshdlr, "sepa_con", 0.0, SCIPinfinity(scip), FALSE, FALSE, TRUE));
            SCIP_CALL(SCIPcacheRowExtensions(scip, row));
            for (auto q = out_edges(*p.first, g); q.first != q.second; ++q.first)
            {
                assert(boost::source(*q.first, g) == *p.first);
                SCIP_CALL(SCIPaddVarToRow(scip, row, x[boost::target(*q.first, g)], 1.0));
            }
            SCIP_CALL(SCIPaddVarToRow(scip, row, x[*p.first], -1.0));
            SCIP_CALL(SCIPflushRowExtensions(scip, row));
            if (SCIPisCutEfficacious(scip, sol, row))
            {
                SCIP_Bool infeasible;
                SCIP_CALL(SCIPaddCut(scip, sol, row, FALSE, &infeasible));
                if (infeasible)
                {
                    *result = SCIP_CUTOFF;
                }
                else
                {
                    *result = SCIP_SEPERATED;
                }
                SCIP_CALL(SCIPreleaseRow(scip, &row));
            }
        }
    }
    return SCIP_OKAY;
}

SCIP_DECL_CONSENFOPS(ConnectivityCons::scip_enfops)
{
    Graph& subgraph = g.create_subgraph();
    size_t num_components = findComponents(scip, 
    if (num_components == 1)
    {
        *result = SCIP_FEASIBLE;
    }
    else
    {
        *result = SCIP_INFEASIBLE
    }
    return SCIP_OKAY;
}

SCIP_DECL_CONSCHECK(ConnectivityCons::scip_check)
{
}
    
SCIP_DECL_CONSLOCK(ConnectivityCons::scip_lock)
{
}
