#pragma once

#include "array2d.h"

struct Pixel
{
    unsigned char r,g,b,a;
};

struct AABB
{
    size_t x1, y1, x2, y2; // inclusive
    inline size_t width() const  { return x2 - x1 + 1; }
    inline size_t height() const { return y2 - y1 + 1; }
};


class Image2d : public Array2d<Pixel>
{
public:
    Image2d();
    Image2d(size_t w, size_t h);
    bool writePNG(const char* fn);
    bool load(const char* fn);
    AABB getAlphaRegion() const;
    void copyscaled(const Image2d& src); // resize this to desired size before calling this
    void maskblit(const Image2d& top);

    Image2d& operator=(const Image2d& o);
};


struct OutlineDraw
{
    Image2d& dst;
    const Image2d& test;
    const Pixel pix;
    OutlineDraw(Image2d& dst, const Image2d& test, Pixel pix) : dst(dst), test(test), pix(pix) {}
    bool operator()(size_t x, size_t y)
    {
        const Pixel here = test(x, y);
        if(here.a)
            return true; // Should never hit a non-fully-transparent pixel
        dst(x,y) = pix;
        return false;
    }
};

struct LineDraw
{
    Image2d& img;
    const Pixel pix;
    LineDraw(Image2d& img, Pixel pix) : img(img), pix(pix) {}
    bool operator()(size_t x, size_t y)
    {
        img(x,y) = pix;
        return false;  // never collide -> always draw the full line
    }
};

