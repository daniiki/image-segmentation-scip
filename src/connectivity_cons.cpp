#include <boost/graph/connected_components.hpp>
#include "connectivity_cons.h"
#include "graph.h"

ConnectivityCons::ConnectivityCons(
    SCIP* scip,
    Graph& g_,
    std::vector<Graph::vertex_descriptor>& T_,
    Graph::vertex_descriptor t_,
    std::vector<SCIP_VAR*>& x_
    ) : 
    ObjConshdlr(scip, "connectivity", "Segemnt connectivity constraints",
        1000000, -2000000, -2000000, 1, -1, 1, 0,
        FALSE, FALSE, TRUE, SCIP_PROPTIMING_BEFORELP, SCIP_PRESOLTIMING_FAST),
    g(g_), T(T_), t(t_), x(x_)
{}

size_t ConnectivityCons::findComponents(
    SCIP* scip,
    SCIP_SOL* sol,
    Graph& subgraph, // subgraph with all superpixels for which x_s is 1
    std::vector<int>& component
    )
{
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (SCIPisEQ(scip, SCIPgetSolVal(scip, sol, x[*p.first]), 1.0))
        {
            add_vertex(*p.first, subgraph);
        }
    }
    size_t num_components = connected_components(subgraph, &component[0]);
    return num_components;
}

SCIP_DECL_CONSENFOLP(ConnectivityCons::scip_enfolp)
{
    Graph& subgraph = g.create_subgraph();
    std::vector<int> component(num_vertices(g));
    size_t num_components = findComponents(scip, NULL, subgraph, component);
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
            if (SCIPisCutEfficacious(scip, NULL, row))
            {
                SCIP_Bool infeasible;
                SCIP_CALL(SCIPaddCut(scip, NULL, row, FALSE, &infeasible));
                if (infeasible)
                {
                    *result = SCIP_CUTOFF;
                }
                else
                {
                    *result = SCIP_SEPARATED;
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
    std::vector<int> component(num_vertices(g));
    size_t num_components = findComponents(scip, NULL, subgraph, component);
    if (num_components == 1)
    {
        *result = SCIP_FEASIBLE;
    }
    else
    {
        *result = SCIP_INFEASIBLE;
    }
    return SCIP_OKAY;
}

SCIP_DECL_CONSCHECK(ConnectivityCons::scip_check)
{
    Graph& subgraph = g.create_subgraph();
    std::vector<int> component(num_vertices(g));
    size_t num_components = findComponents(scip, sol, subgraph, component);
    if (num_components == 1)
    {
        *result = SCIP_FEASIBLE;
    }
    else
    {
        *result = SCIP_INFEASIBLE;
    }
    return SCIP_OKAY;

}
    
SCIP_DECL_CONSLOCK(ConnectivityCons::scip_lock)
{
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (std::find(T.begin(), T.end(), *p.first) == T.end())
        {
            // The variable x_s affects connectivity iff s is not in T,
            // since x_t=1 and x_s=0 for s in T\{t} are given
            // Connectivity can only be lost when lowering a variable value,
            // therefore nlockspos, nlockneg
            SCIP_CALL(SCIPaddVarLocks(scip, x[*p.first], nlockspos, nlocksneg));
        }
    }
}
