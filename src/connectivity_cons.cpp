#include <boost/graph/connected_components.hpp>
#include "connectivity_cons.h"
#include "graph.h"

ConnectivityCons::ConnectivityCons(
    SCIP* scip,
    Graph& g_,
    std::vector<Graph::vertex_descriptor>& master_nodes_,
    Graph::vertex_descriptor master_node_,
    std::vector<SCIP_VAR*>& superpixel_vars_
    ) :
    ObjConshdlr(scip, "connectivity", "Segemnt connectivity constraints",
        1000000, -2000000, -2000000, 1, -1, 1, 0,
        FALSE, FALSE, TRUE, SCIP_PROPTIMING_BEFORELP, SCIP_PRESOLTIMING_FAST),
    g(g_), master_nodes(master_nodes_), master_node(master_node_), superpixel_vars(superpixel_vars_)
{}

SCIP_DECL_CONSTRANS(ConnectivityCons::scip_trans)
{
    SCIP_CALL(SCIPcreateCons(scip, targetcons, SCIPconsGetName(sourcecons), conshdlr, NULL,
        SCIPconsIsInitial(sourcecons), SCIPconsIsSeparated(sourcecons), SCIPconsIsEnforced(sourcecons),
        SCIPconsIsChecked(sourcecons), SCIPconsIsPropagated(sourcecons),  SCIPconsIsLocal(sourcecons),
        SCIPconsIsModifiable(sourcecons), SCIPconsIsDynamic(sourcecons), SCIPconsIsRemovable(sourcecons),
        SCIPconsIsStickingAtNode(sourcecons)));

    return SCIP_OKAY;
}

size_t ConnectivityCons::findComponents(
    SCIP* scip,
    SCIP_SOL* sol,
    Graph& subgraph,
    std::vector<int>& component
    )
{
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        if (SCIPisEQ(scip, SCIPgetSolVal(scip, sol, superpixel_vars[*p.first]), 1.0))
        {
            add_vertex(*p.first, subgraph);
        }
    }
    size_t num_components = connected_components(subgraph, &component[0]);
    return num_components;
}

SCIP_RETCODE ConnectivityCons::sepaConnectivity(
    SCIP* scip,
    SCIP_CONSHDLR* conshdlr,
    SCIP_SOL* sol,
    SCIP_RESULT* result
    )
{
    Graph& subgraph = g.create_subgraph();
    std::vector<int> component(num_vertices(g));
    size_t num_components = findComponents(scip, sol, subgraph, component);
    if (num_components == 1)
    {
        *result = SCIP_DIDNOTFIND;
    }
    else
    {
        for (int i = 0; i < num_components; i++)
        {
            if (i == component[master_node])
            {
                continue;
            }
            
            std::vector<Graph::vertex_descriptor> superpixels; // all superpixels in the component
            for (auto p = vertices(subgraph); p.first != p.second; ++p.first)
            {
                if (component[*p.first] == i)
                {
                    superpixels.push_back(*p.first);
                }
            }

            // add a constraint/row to the problem
            SCIP_ROW* row;
            SCIP_CALL(SCIPcreateEmptyRowCons(scip, &row, conshdlr, "sepa_con", 0.0, SCIPinfinity(scip), FALSE, FALSE, TRUE));
            //SCIP_CALL(SCIPcreateEmptyRowCons(scip, &row, conshdlr, "sepa_con", -superpixels.size() + 1, SCIPinfinity(scip), FALSE, FALSE, TRUE));
            SCIP_CALL(SCIPcacheRowExtensions(scip, row));

            // sum_{all superpixels s surrounding the component} x_s >= ...
            for (auto s : superpixels)
            {
                for (auto q = out_edges(s, g); q.first != q.second; ++q.first)
                {
                    assert(boost::source(*q.first, g) == s);
                    // if the tartget is not in the component
                    if (std::find(
                            superpixels.begin(),
                            superpixels.end(),
                            boost::target(*q.first, g)
                        ) == superpixels.end())
                    {
                        SCIP_CALL(SCIPaddVarToRow(scip, row, superpixel_vars[boost::target(*q.first, g)], 1.0));
                    }
                }
            }

            // ... >= x_{a random superpixel in the component}
            SCIP_CALL(SCIPaddVarToRow(scip, row, superpixel_vars[superpixels[0]], -1.0));

            /*for (auto s : superpixels)
            {
                SCIP_CALL(SCIPaddVarToRow(scip, row, superpixel_vars[s], -1.0));
            }*/

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
            }
            SCIP_CALL(SCIPreleaseRow(scip, &row));
        }
    }
    delete &subgraph; // delete subgraph, and thereby free memory
    g.m_children.clear();
    return SCIP_OKAY;
}

SCIP_DECL_CONSSEPALP(ConnectivityCons::scip_sepalp)
{
    SCIP_CALL(sepaConnectivity(scip, conshdlr, NULL, result));
    return SCIP_OKAY;
}

SCIP_DECL_CONSSEPASOL(ConnectivityCons::scip_sepasol)
{
    SCIP_CALL(sepaConnectivity(scip, conshdlr, sol, result));
    return SCIP_OKAY;
}

SCIP_DECL_CONSENFOLP(ConnectivityCons::scip_enfolp)
{
    Graph& subgraph = g.create_subgraph();
    std::vector<int> component(num_vertices(g));
    size_t num_components = findComponents(scip, NULL, subgraph, component);
    delete &subgraph; // delete subgraph, and thereby free memory
    g.m_children.clear();
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

SCIP_DECL_CONSENFOPS(ConnectivityCons::scip_enfops)
{
    Graph& subgraph = g.create_subgraph();
    std::vector<int> component(num_vertices(g));
    size_t num_components = findComponents(scip, NULL, subgraph, component);
    delete &subgraph; // delete subgraph, and thereby free memory
    g.m_children.clear();
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
    delete &subgraph; // delete subgraph, and thereby free memory
    g.m_children.clear();
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
        if (std::find(master_nodes.begin(), master_nodes.end(), *p.first) == master_nodes.end())
        {
            // The variable x_s affects connectivity iff s is not in master_nodes,
            // since x_t=1 and x_s=0 for s in master_nodes\{t} are given
            // Connectivity can only be lost when lowering a variable value,
            // therefore nlockspos, nlockneg
            SCIP_CALL(SCIPaddVarLocks(scip, superpixel_vars[*p.first], nlockspos + nlocksneg, nlockspos + nlocksneg));
        }
    }
}

SCIP_RETCODE SCIPcreateConsConnectivity(
    SCIP* scip,
    SCIP_CONS** cons,
    const char* name,
    SCIP_Bool initial,
    SCIP_Bool separate,
    SCIP_Bool enforce,
    SCIP_Bool check,
    SCIP_Bool propagate,
    SCIP_Bool local,
    SCIP_Bool modifiable,
    SCIP_Bool dynamic,
    SCIP_Bool removable
    )
{
    SCIP_CONSHDLR* conshdlr;
    SCIP_CONSDATA* consdata = NULL;

    // find the connectiviity constraint handler
    conshdlr = SCIPfindConshdlr(scip, "connectivity");
    if (conshdlr == NULL)
    {
        SCIPerrorMessage("connectivity constraint handler not found\n");
        return SCIP_PLUGINNOTFOUND;
    }

    // create constraint 
    SCIP_CALL(SCIPcreateCons(scip, cons, name, conshdlr, consdata, initial, separate, enforce, check, propagate,
        local, modifiable, dynamic, removable, FALSE));

   return SCIP_OKAY;
}
