#pragma once

#include <iostream>

//#include "LOGO_DEPFET.h"
#include "LOGO_DSSCXFEL.h"

/*  Call this macro repeatedly.  After each use, the pixel data can be extracted  */

#define LOGO_HEADER_PIXEL(data,pixel) {\
pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \
pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \
pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \
data += 4; \
}

namespace LOGO{
  static const uint32_t height = 128;
  static const uint32_t width  = 512;
  constexpr static const unsigned int numpxs = width*height;

  static bool image[numpxs];

  static void print(uint32_t w){
    for(uint32_t i=0; i<height; i++){
      uint32_t row = height - i - 1;
      for(uint32_t col=0; col<w; col++){
        std::cout << (image[row*width + col]? "$" : " ");
      }
      std::cout << std::endl;
    }
  }

  static void init()
  {
    int pixel[3];

    if(false) print(width);

    bool imageData[numpxs];
    auto data = LOGO_DSSCXFEL::data;

    uint32_t px = 0;
    for(uint32_t i=0; i<height; i++){
      for(uint32_t j=0; j<width; j++){
        LOGO_HEADER_PIXEL(data,pixel);
        //std::cout << "(" << pixel[0] << "/" << pixel[1] << "/" << pixel[2] << ")" << std::endl;
        imageData[px] = pixel[0]==0;
        px++;
      }
    }

    px = 0;
    for(uint32_t i=0; i<height; i++){
      for(uint32_t j=0; j<width; j++){
        uint32_t imgIdx = (height-i-1)*512 + j;
        image[imgIdx] = imageData[px++];
      }
    }
  }
}
