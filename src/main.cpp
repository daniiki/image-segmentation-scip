/** @file */ 

#include "graph.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/connected_components.hpp>
#include "pricer.h"

#include <scip/scipdefplugins.h>

#include <iostream>
#include <math.h>
#include <string>

#include "vardata.h"
#include "image.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

/**
 * Setup and solve the master problem
 */
SCIP_RETCODE master_problem(
    Graph& g, ///< the graph of superpixels
    std::vector<Graph::vertex_descriptor> master_nodes, ///< master nodes of all segments 
    std::vector<std::set<Graph::vertex_descriptor>> initial_segments, ///< an initial set of segments that will be added as variables
                                                                      ///< These should form a feasible solution, but do not need to be connected.
    std::vector<std::vector<Graph::vertex_descriptor>>& segments ///< the selected segments will be stored in here
)
{
    SCIP* scip;
    SCIP_CALL(SCIPcreate(& scip));
    SCIP_CALL(SCIPincludeDefaultPlugins(scip));
    SCIP_CALL(SCIPsetIntParam(scip, "display/verblevel", 5));
    SCIP_CALL(SCIPsetIntParam(scip, "presolving/maxrestarts", 0)); // see Known Bugs at http://scip.zib.de/#contact
    
    //create master problem
    SCIP_CALL(SCIPcreateProb(scip, "master_problem", NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    SCIP_CALL(SCIPsetObjsense(scip, SCIP_OBJSENSE_MINIMIZE));
    
    std::vector<SCIP_VAR*> vars;
    // vars[i] belongs to segment initial_segments[i]
    for (auto segment : initial_segments)
    {
        SCIP_VAR* var;
        // Set a very high objective value for the initial segments so that they aren't selected in the final solution
        SCIP_CALL(SCIPcreateVar(scip, &var, "x_P", 0.0, 1.0, 10000, SCIP_VARTYPE_BINARY, TRUE, FALSE, NULL, NULL, NULL, NULL, NULL));
        SCIP_CALL(SCIPaddVar(scip, var));
        vars.push_back(var);
    }
    
    std::vector<SCIP_CONS*> partitioning_cons;
    for (auto p = vertices(g); p.first != p.second; ++p.first)
    {
        SCIP_CONS* cons1;
        SCIP_CALL(SCIPcreateConsLinear(scip, & cons1, "first", 0, NULL, NULL, 1.0, 1.0,
                     true,                   /* initial */
                     false,                  /* separate */
                     true,                   /* enforce */
                     true,                   /* check */
                     true,                   /* propagate */
                     false,                  /* local */
                     true,                   /* modifiable */
                     false,                  /* dynamic */
                     false,                  /* removable */
                     false) );               /* stickingatnode */
        for (size_t i = 0; i != initial_segments.size(); ++i)
        {
            if (initial_segments[i].find(*p.first) != initial_segments[i].end())
            {
                SCIP_CALL(SCIPaddCoefLinear(scip, cons1, vars[i], 1.0));
            }
        }
        SCIP_CALL(SCIPaddCons(scip, cons1));
        partitioning_cons.push_back(cons1);
    }
    
    SCIP_CONS* num_segments_cons;
    size_t k = initial_segments.size();
    SCIP_CALL(SCIPcreateConsLinear(scip, &num_segments_cons, "second", 0, NULL, NULL, k, k,
                     true,                   /* initial */
                     false,                  /* separate */
                     true,                   /* enforce */
                     true,                   /* check */
                     true,                   /* propagate */
                     false,                  /* local */
                     true,                   /* modifiable */
                     false,                  /* dynamic */
                     false,                  /* removable */
                     false) );               /* stickingatnode */
    for (size_t i = 0; i != initial_segments.size(); ++i)
    {
        SCIP_CALL(SCIPaddCoefLinear(scip, num_segments_cons, vars[i], 1.0));
    }
    SCIP_CALL(SCIPaddCons(scip, num_segments_cons));
    
    // include pricer 
    SegmentPricer* pricer_ptr = new SegmentPricer(scip, g, master_nodes, partitioning_cons, num_segments_cons);
    SCIP_CALL(SCIPincludeObjPricer(scip, pricer_ptr, true));
    
    // activate pricer 
    SCIP_CALL(SCIPactivatePricer(scip, SCIPfindPricer(scip, "fitting_pricer")));
    
    // solve
    SCIP_CALL(SCIPsolve(scip));
    SCIP_SOL* sol = SCIPgetBestSol(scip);

    // return selected segments
    SCIP_VAR** variables = SCIPgetVars(scip);
    for (int i = 0; i < SCIPgetNVars(scip); ++i)
    {
        if (SCIPisEQ(scip, SCIPgetSolVal(scip, sol, variables[i]), 1.0))
        {
            auto vardata = (ObjVardataSegment*) SCIPgetObjVardata(scip, variables[i]);
            segments.push_back(vardata->getSuperpixels());
        }
    }

    // check if the selected segments are connected
    for (auto segment : segments)
    {
        Graph& subgraph = g.create_subgraph();
        std::vector<int> component(num_vertices(g));
        for (auto superpixel : segment)
        {
            add_vertex(superpixel, subgraph);
        }
        assert(connected_components(subgraph, &component[0]) == 1);
    }
    
    return SCIP_OKAY;
}

std::vector<std::pair<uint32_t, uint32_t>> master_pixels;

static void onMouse(int event, int x, int y, int f, void*)
{
    if (event == CV_EVENT_LBUTTONDOWN)
    {
        std::cout << x << " " << y << std::endl;
        master_pixels.push_back(std::pair<uint32_t, uint32_t>(x, y));
    }
}

/**
 * The main function reads the image, retrieves the graph of superpixels, solves the master problem and outputs the solution.
 */
int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "Usage: bin/fopra input.png num_superpixels" << std::endl;
        return 1;
    }
    Image image(argv[1], std::stoi(argv[2]));

    Mat img = imread("superpixels_avgcolor.png");
    namedWindow("Select master nodes");
    setMouseCallback("Select master nodes", onMouse, 0);
    imshow("Select master nodes", img);
    waitKey(0);
    cvDestroyWindow("Select master nodes");
    
    std::vector<Graph::vertex_descriptor> master_nodes;
    for (auto xy : master_pixels)
    {
        Graph::vertex_descriptor superpixel = image.pixelToSuperpixel(xy.first, xy.second);
        if (std::find(master_nodes.begin(), master_nodes.end(), superpixel) == master_nodes.end())
        {
            master_nodes.push_back(superpixel);
        }
    }

    Graph g = image.graph();
    size_t n = num_vertices(g);
    std::vector<std::set<Graph::vertex_descriptor>> initial_segments;
    for (size_t i = 1; i < master_nodes.size(); ++i)
    {
        std::set<Graph::vertex_descriptor> segment;
        segment.insert(master_nodes[i]);
        initial_segments.push_back(segment);
    }
    std::set<Graph::vertex_descriptor> segment;
    for (uint32_t i = 0; i < n; ++i)
    {
        if (i == master_nodes[0])
        {
            segment.insert(i);
        }
        else if (std::find(master_nodes.begin(), master_nodes.end(), i) == master_nodes.end())
        {
            segment.insert(i);
        }
    }
    initial_segments.push_back(segment);

    std::cout << "Selecting " << initial_segments.size() << " segments" << std::endl;

    std::vector<std::vector<Graph::vertex_descriptor>> segments; // the selected segments will be stored in here
    SCIP_CALL(master_problem(g, master_nodes, initial_segments, segments));
    image.writeSegments(master_nodes, segments, g);

    img = imread("segments.png");
    namedWindow("Selected segments");
    imshow("Selected segments", img);
    waitKey(0);
    cvDestroyWindow("Selected segments");

    return 0;
}
