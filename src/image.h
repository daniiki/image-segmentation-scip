#ifndef IMAGE_H
#define IMAGE_H

class Image {
public:
    // n is the desired number of superpixels
    Image(std::string filename, int n);
    
    Graph* graph();

    void writeSegments(std::vector<Graph::vertex_descriptor> T, std::vector<std::vector<Graph::vertex_descriptor>> segments, Graph& g);
    
private:
    unsigned int width;
    unsigned int height;
    unsigned int superpixelcount;
    uint32_t* segmentation;
    std::vector<double> avgcolor;
};

#endif
