//#include "SLIC/SCLIC.h"
#include <png++/png.hpp>
#include <bitset>
#include <iostream>
#include <typeinfo>

void readimage()
{
    png::image<png::rgb_pixel> image("input.png");
    
    uint32_t pbuff[image.get_height()*image.get_width()];
    
    for (png::uint_32 y = 0; y < image.get_height(); ++y)
    {
        for (png::uint_32 x = 0; x < image.get_width(); ++x)
        {
            uint32_t argb = image[y][x].red << 16
	        | image[y][x].green << 8
	        | image[y][x].blue << 0;
            pbuff[x + y*image.get_width()] = argb;
        }
    }
}

int main()
{
    readimage();
}
