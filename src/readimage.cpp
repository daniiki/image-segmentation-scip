//#include "SLIC/SCLIC.h"
#include <png++/png.hpp>
#include <bitset>
#include <iostream>

void readimage()
{
    png::image<png::rgb_pixel> image("input.png");
    
    uint32_t pbuff[image.get_height()*image.get_width()];
    
    for (png::uint_32 y = 0; y < image.get_height(); ++y)
    {
        for (png::uint_32 x = 0; x < image.get_width(); ++x)
        {
            std::cout<<std::bitset<8>(image[x][y].red)<<std::endl;
            std::cout<< (uint16_t) image[x][y].red << std::endl;
            uint32_t argb = (uint16_t) image[x][y].red;
            pbuff[x + y*image.get_width()] = argb;
        }
    }
}

int main()
{
    readimage();
}
