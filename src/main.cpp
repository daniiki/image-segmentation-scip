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

#include "vardata.h"
#include "image.h"

#include <GL/glut.h>
#include <IL/il.h>

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
        size_t num_components = connected_components(subgraph, &component[0]);
        assert(num_components == 1);
    }
    
    return SCIP_OKAY;
}

#define DEFAULT_WIDTH  640
#define DEFAULT_HEIGHT 480

int width  = DEFAULT_WIDTH;
int height = DEFAULT_HEIGHT;

/* Handler for window-repaint event. Called back when the window first appears and
   whenever the window needs to be re-painted. */
void display()
{
    // Clear color and depth buffers
       glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
       glMatrixMode(GL_MODELVIEW);     // Operate on model-view matrix

    /* Draw a quad */
       glBegin(GL_QUADS);
           glTexCoord2i(0, 0); glVertex2i(0,   0);
           glTexCoord2i(0, 1); glVertex2i(0,   height);
           glTexCoord2i(1, 1); glVertex2i(width, height);
           glTexCoord2i(1, 0); glVertex2i(width, 0);
       glEnd();

    glutSwapBuffers();
}

/* Handler for window re-size event. Called back when the window first appears and
   whenever the window is re-sized with its new width and height */
void reshape(GLsizei newwidth, GLsizei newheight)
{
    // Set the viewport to cover the new window
       glViewport(0, 0, width=newwidth, height=newheight);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();
     glOrtho(0.0, width, height, 0.0, 0.0, 100.0);
     glMatrixMode(GL_MODELVIEW);

    glutPostRedisplay();
}

/* Initialize OpenGL Graphics */
void initGL(int w, int h)
{
     glViewport(0, 0, w, h); // use a screen size of WIDTH x HEIGHT
     glEnable(GL_TEXTURE_2D);     // Enable 2D texturing

    glMatrixMode(GL_PROJECTION);     // Make a simple 2D projection on the entire window
     glLoadIdentity();
     glOrtho(0.0, w, h, 0.0, 0.0, 100.0);

     glMatrixMode(GL_MODELVIEW);    // Set the matrix mode to object modeling

     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
     glClearDepth(0.0f);
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the window
}

/**
 * The main function reads the image, retrieves the graph of superpixels, solves the master problem and outputs the solution.
 */
int main(int argc, char** argv)
{
    Image image("src/input.png", 30);

    // https://www.opengl.org/discussion_boards/showthread.php/181714-Does-opengl-help-in-the-display-of-an-existing-image
    /* GLUT init */
    glutInit(&argc, argv);            // Initialize GLUT
       glutInitDisplayMode(GLUT_DOUBLE); // Enable double buffered mode
       glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);   // Set the window's initial width & height
       glutCreateWindow(argv[0]);      // Create window with the name of the executable
       glutDisplayFunc(display);       // Register callback handler for window re-paint event
       glutReshapeFunc(reshape);       // Register callback handler for window re-size event
 
    /* OpenGL 2D generic init */
    initGL(DEFAULT_WIDTH, DEFAULT_HEIGHT);

    ILboolean success;
    ILuint sup_img;
    ilInit();
    ilGenImages(1, &sup_img); /* Generation of one image name */
    ilBindImage(sup_img); /* Binding of image name */
    const char* filename = "superpixels.png";
    success = ilLoadImage(filename); /* Loading of the image filename by DevIL */
    if (!success)
        return -1;
    /* Convert every colour component into unsigned byte. If your image contains alpha channel you can replace IL_RGB with IL_RGBA */
    success = ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    if (!success)
        return -1;

    GLuint texid;
    /* OpenGL texture binding of the image loaded by DevIL  */
    glGenTextures(1, &texid); /* Texture name generation */
    glBindTexture(GL_TEXTURE_2D, texid); /* Binding of texture name */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); /* We will use linear interpolation for magnification filter */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); /* We will use linear interpolation for minifying filter */
    glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT), 
        0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE, ilGetData()); /* Texture specification */

    glutMainLoop();

    /* Main loop */
    glutMainLoop();

    /* Delete used resources and quit */
     ilDeleteImages(1, &sup_img); /* Because we have already copied image data into texture data we can release memory used by image. */
     glDeleteTextures(1, &texid);
    
    Graph g = image.graph();
    size_t n = num_vertices(g);
    size_t k = 5; // number of segments to cover the image with
    size_t m = n / k;
    std::vector<Graph::vertex_descriptor> master_nodes(k);
    std::vector<std::set<Graph::vertex_descriptor>> inital_segments(k);
    for (size_t i = 0; i < k; i++)
    {
        master_nodes[i] = i * m;
    }
    for (size_t i = 0; i < k - 1; i++)
    {
        for (size_t j = master_nodes[i]; j < master_nodes[i+1]; ++j)
        {
            inital_segments[i].insert(j);
        }
    }
    for (size_t j = master_nodes[k-1]; j < n; ++j)
    {
        inital_segments[k-1].insert(j);
    }

    std::vector<std::vector<Graph::vertex_descriptor>> segments; // the selected segments will be stored in here
    SCIP_CALL(master_problem(g, master_nodes, inital_segments, segments));
    image.writeSegments(master_nodes, segments, g);
    return 0;
}
