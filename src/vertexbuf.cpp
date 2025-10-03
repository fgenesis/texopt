#include "vertexbuf.h"
#include "polygon.h"
#include "stripifier.h"


size_t genIndexBuffer_Strip(std::vector<unsigned>& dst, const Polygon* polys, size_t n, bool useRestart)
{
    if(!n)
        return 0;

    size_t ret = 0;
    size_t vertexcount = 0;
    std::vector<unsigned> indices;
    {
        std::vector<Tri> tris;
        for(size_t i = 0; i < n; ++i)
        {
            tris.clear();
            polys[i].triangulate(tris);
            for(size_t k = 0; k < tris.size(); ++k)
            {
                indices.push_back(vertexcount + tris[k].a);
                indices.push_back(vertexcount + tris[k].b);
                indices.push_back(vertexcount + tris[k].c);
            }
            vertexcount += polys[i].points.size();
        }
    }

    assert(vertexcount > 2);

    return stripify(dst, &indices[0], indices.size(), vertexcount, useRestart ? RESTART : 0);
}

void polygonPointsToVertexList(std::vector<uvec2>& points, ivec2 offset, const Polygon* polys, size_t n)
{
    for(size_t i = 0; i < n; ++i)
    {
        const Polygon& poly = polys[i];
        const size_t pts = poly.points.size();
        for(size_t k = 0; k < pts; ++k)
        {
            const Point2d& p = poly.points[k];
            points.push_back(uvec2(p.x, p.y) + uvec2(offset));
        }
    }
}

void indexListToTris(std::vector<Tri>& tris, const unsigned* indices, size_t n, TriMode trimode)
{
    size_t first = 0;
    for(size_t i = 2; i < n; )
    {
        if(indices[i] == RESTART)
        {
            first = i + 1;
            i += 3;
            continue;
        }
        Tri tri;
        switch(trimode)
        {
            case TRIMODE_STRIP:
                tri.a = indices[i-2];
                tri.b = indices[i-1];
                tri.c = indices[i];
                break;
            case TRIMODE_FAN:
                tri.a = indices[first];
                tri.b = indices[i-1];
                tri.c = indices[i];
                break;
        }

        if(!(tri.a == tri.b || tri.b == tri.c || tri.c == tri.a)) // not degenerate?
            tris.push_back(tri);

        ++i;
    }
}

// warning: probably broken, don't use
size_t strip2fan(std::vector<unsigned>& dst, const unsigned* stripIndexes, size_t n)
{
    unsigned maxval = 0;
    for(size_t i = 0; i < n; ++i)
        maxval = std::max(maxval, stripIndexes[i]);

    dst.clear();
    dst.resize(maxval+1, 0);
    for(size_t i = 0; i < n; ++i)
        ++dst[stripIndexes[i]];

    bool hasmax = false;
    size_t maxidx = 0;
    for(size_t i = 0; i <= maxval; ++i)
    {
        assert(dst[i]);
        if(dst[i] > 1)
        {
            if(hasmax)
                return 0;
            else
            {
                hasmax = true;
                maxidx = i;
            }
        }
    }
    assert(hasmax);

    dst.clear();
    dst.push_back(maxidx);

    for(size_t i = 0; i < n; ++i)
        if(stripIndexes[i] != maxidx)
            dst.push_back(stripIndexes[i]);

    return dst.size();
}
