#include "vertexbuf.h"
#include "polygon.h"
#include "stripifier.h"

enum { RESTART = 0xffff };


size_t genIndexBuffer_Strip(std::vector<unsigned>& dst, const Polygon* polys, size_t n)
{
    if(!n)
        return 0;

    size_t ret = 0;
    size_t vertexcount = 0;
    std::vector<unsigned> indices;
    {
        std::vector<Tri> tris;
        size_t begin = 0;
        for(size_t i = 0; i < n; ++i)
        {
            tris.clear();
            polys[i].triangulate(tris);
            for(size_t k = 0; k < tris.size(); ++k)
            {
                indices.push_back(begin + tris[k].a);
                indices.push_back(begin + tris[k].b);
                indices.push_back(begin + tris[k].c);
            }
            begin += indices.size();
            vertexcount += polys[i].points.size();
        }
    }

    return stripify(dst, &indices[0], indices.size(), vertexcount, RESTART);
}
