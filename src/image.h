#ifndef IMAGE_H
#define IMAGE_H

class Image {
public:
    // n is the desired number of superpixels
    Image(std::string filename, int n);
    
    Graph* graph();
    
private:
    unsigned int width;
    unsigned int height;
    unsigned int superpixelcount;
    uint32_t* segmentation;
    std::vector<double> avgcolor;
};

#endif