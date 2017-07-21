#include <vl/slic.h>
#include <png++/png.hpp>
#include <iostream>
#include <cmath>
#include "graph.h"
#include "image.h"


Image::Image(std::string filename, int n)
{
    png::image<png::gray_pixel> pngimage(filename);
    width = pngimage.get_width();
    height = pngimage.get_height();
    unsigned int imagesize = pngimage.get_width() * pngimage.get_height();
    
    float* image = new float[imagesize];
    for (png::uint_32 x = 0; x < pngimage.get_width(); ++x)
    {
        for (png::uint_32 y = 0; y < pngimage.get_height(); ++y)
        {
            image[x + y * pngimage.get_width()] = pngimage[y][x] / 255.0;
        }
    }
    
    segmentation = new vl_uint32[imagesize];
    vl_slic_segment(
        segmentation,
        image,
        pngimage.get_width(),
        pngimage.get_height(),
        1, // numChannels
        sqrt(imagesize / n), // regionSize
        10.0, // regularization
        0 // minRegionSize
    );
    
    superpixelcount = 0;
    for (size_t i = 0; i < imagesize; ++i)
    {
        if (superpixelcount < segmentation[i])
        {
            superpixelcount = segmentation[i];
        }
    }
    superpixelcount++;
    std::cout << "Generated " << superpixelcount << " superpixels." << std::endl;
    
    avgcolor.resize(superpixelcount, 0.0);
    std::vector<unsigned int> numpixels(superpixelcount, 0);
    for (png::uint_32 x = 0; x < pngimage.get_width(); ++x)
    {
        for (png::uint_32 y = 0; y < pngimage.get_height(); ++y)
        {
            avgcolor[segmentation[x + y * pngimage.get_width()]] += pngimage[y][x];
            numpixels[segmentation[x + y * pngimage.get_width()]] += 1;
        }
    }
    for (size_t i = 0; i < superpixelcount; ++i)
    {
        avgcolor[i] /= numpixels[i];
    }
    
    for (png::uint_32 x = 0; x < pngimage.get_width(); ++x)
    {
        for (png::uint_32 y = 0; y < pngimage.get_height(); ++y)
        {
            pngimage[y][x] = avgcolor[segmentation[x + y * pngimage.get_width()]];
        }
    }
    pngimage.write("superpixels.png");
}

Graph* Image::graph()
{
    auto g = new Graph(superpixelcount);
    for (unsigned int i = 0; i < superpixelcount; ++i)
    {
        (*g)[i].color = avgcolor[i];
    }
    // add edges
    for (png::uint_32 x = 0; x < width; ++x)
    {
        for (png::uint_32 y = 0; y < height; ++y)
        {
            auto current = x + y * width;
            auto right = x + 1 + y * width;
            auto below = x + (y + 1) * width;
            if (x + 1 < width
                && segmentation[current] != segmentation[right])
            {
                // add edge to the superpixel on the right
                add_edge(segmentation[current], segmentation[right], *g);
            }
            if (y + 1 < height
                && segmentation[current] != segmentation[below])
            {
                // add edge to the superpixel below
                add_edge(segmentation[current], segmentation[below], *g);
            }
        }
    }
    return g;
}

void Image::writeSegments(std::vector<Graph::vertex_descriptor> T, std::vector<std::vector<Graph::vertex_descriptor>> segments, Graph& g)
{
    png::image<png::gray_pixel> image(width, height);
    for (png::uint_32 x = 0; x < width; ++x)
    {
        for (png::uint_32 y = 0; y < height; ++y)
        {
            auto superpixel = segmentation[x + y*width];
	    auto segment = segments.begin();
	    while (std::find(segment->begin(), segment->end(), superpixel) == segment->end())
                ++segment;
            auto t = T.begin();
	    while (std::find(segment->begin(), segment->end(), *t) == segment->end())
		++t;
	    image[y][x] = g[*t].color;
	}
    }
    std::cout << "write segments.png" << std::endl;
    image.write("segments.png");
}
