#ifndef IMAGE_H
#define IMAGE_H

/**
 * Class representing png image
 */
class Image {
public:
    Image(
        std::string filename, ///< PNG image to read
        int n ///< desired number of superpixels
        );
    
    /**
     * Creates a Boost graph consisting of the generated superpixels
     * The number of adjacent pixels between superpixels is stored as edge weight.
     */
    Graph graph(); 

    /*
     * Writes segements into segments.png
     */
    void writeSegments(
        std::vector<Graph::vertex_descriptor> master_nodes,  ///< master nodes of all segments 
        std::vector<std::vector<Graph::vertex_descriptor>> segments, ///< optimal segmentation, where each segment is a vector consisting of the superpixels contained in it
        Graph& g ///< the graph of superpixels 
        );
    
private:
    unsigned int width;
    unsigned int height;
    unsigned int superpixelcount;
    uint32_t* segmentation;
    std::vector<double> avgcolor;
    std::string filename;
};

#endif
