#pragma once

#include "array2d.h"

struct Pixel
{
    unsigned char r,g,b,a;
};

struct AABB
{
    size_t x1, y1, x2, y2; // inclusive
    inline size_t width() const  { return x2 - x1; }
    inline size_t height() const { return y2 - y1; }
};


class Image2d : public Array2d<Pixel>
{
public:
    Image2d();
    Image2d(size_t w, size_t h);
    bool writePNG(const char* fn);
    bool load(const char* fn);
    AABB getAlphaRegion() const;

};
