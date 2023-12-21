#include "atlas.h"
#include "vertexbuf.h"

void Atlas::add(const Image2d& img, Polygon* polys, size_t n)
{
    AtlasFragment frag;
    frag.img = img;

    size_t striplen = genIndexBuffer_Strip(frag.strip, polys, n);

    for(size_t i = 0; i < n; ++n)
    {
        const std::vector<Point2d>& p = polys[i].points;
        for(size_t k = 0; k < p.size(); ++k)
        {
            ivec2 v { p[k].x, p[k].y };
            frag.vtx.push_back(v);
        }
    }

    // TODO: fill occupied triangle area
    //
    // to place:
    // check polygon-vs-occupied target pixmap first (lines vs pixels)
    // use dt for scoring, abort if pixel is occupied

    frags.push_back(frag);
}

void Atlas::build()
{
    for(size_t i = 0; i < frags.size(); ++i)
    {
    }
}
