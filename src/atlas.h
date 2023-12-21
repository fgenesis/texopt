#pragma once

#include "polygon.h"
#include "image2d.h"

struct AtlasFragment
{
    Image2d img;
    std::vector<unsigned> strip;
    std::vector<ivec2> vtx;
};

class Atlas
{
    void add(const Image2d& img, Polygon *polys, size_t n);

    void build();

private:
    std::vector<AtlasFragment> frags;
};
