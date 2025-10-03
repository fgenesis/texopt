#include "trifill.h"
#include <algorithm>
#include <set>
#include "vec.h"
#include "algo2d.h"
#include <assert.h>
#include <string.h>
#include "array2d.h"
#include "polygon.h"
#include "image2d.h"

struct PointCollector
{
    std::vector<uvec2>& points;
    PointCollector(std::vector<uvec2>& dst) : points(dst) {}

    inline bool operator()(size_t x, size_t y) const
    {
        points.push_back(uvec2(x, y));
        return false; // don't collide, just continue
    }

};

static void trace(std::vector<uvec2>& out, uvec2 start, uvec2 end)
{
    PointCollector c(out);
    linecast(start, end, c, NULL);
}

static bool pointOrder(const uvec2& a, const uvec2& b)
{
    return a.y < b.y || a.y == b.y && a.x < b.x;
}

static void fillpoints(std::vector<uvec2>& pointsWrk, uvec2 a, uvec2 b, uvec2 c)
{
    // Kinda wasteful to do it this way but whatev
    pointsWrk.clear();
    trace(pointsWrk, a, b);
    trace(pointsWrk, b, c);
    trace(pointsWrk, c, a);
    std::sort(pointsWrk.begin(), pointsWrk.end(), pointOrder);
}

static void dotri(Array2d<unsigned char>& out, uvec2 a, uvec2 b, uvec2 c, std::vector<uvec2>& pointsWrk)
{
    fillpoints(pointsWrk, a, b, c);
    const size_t N = pointsWrk.size();
    uvec2 start, end;
    for(size_t i = 0; i < N; )
    {
        end = start = pointsWrk[i++];
        while(i < N && pointsWrk[i].y == start.y)
            end = pointsWrk[i++];

        assert(start.y == end.y);
        assert(!(i < N) || pointsWrk[i].y == start.y+1); // exactly one greater, can't have a gap
        // i is now at first point with greater y

        unsigned char *rowptr = out.row(start.y);
        memset(rowptr + start.x, 1, end.x - start.x + 1);
    }
}

// Fill area covered by triangles in uv, return number of filled pixels
size_t trifill(Array2d<unsigned char>& out, const uvec2 *points, const Tri *tris, size_t ntris)
{
    std::vector<uvec2> pointsWrk;
    pointsWrk.reserve(128); // guess
    for(size_t i = 0; i < ntris; ++i)
    {
        Tri t = tris[i];
        dotri(out, points[t.a], points[t.b], points[t.c], pointsWrk);
    }

    // need to count this afterwards since triangle edges overlap
    size_t filled = 0;
    const size_t N = out.width() * out.height();
    const unsigned char *p = out.data();
    for(size_t i = 0; i < N; ++i)
        filled += !!p[i];

    return filled;
}

static void drawtri(Image2d& out, ivec2 offset, uvec2 a, uvec2 b, uvec2 c, std::vector<uvec2>& pointsWrk, const Image2d& src)
{
    fillpoints(pointsWrk, a, b, c);
    const size_t N = pointsWrk.size();
    uvec2 start, end;
    for(size_t i = 0; i < N; )
    {
        end = start = pointsWrk[i++];
        while(i < N && pointsWrk[i].y == start.y)
            end = pointsWrk[i++];

        assert(start.y == end.y);
        assert(!(i < N) || pointsWrk[i].y == start.y+1); // exactly one greater, can't have a gap
        // i is now at first point with greater y

        const Pixel *psrc = src.row(start.y) + start.x;
        Pixel *pdst = out.row(size_t(start.y) + offset.y) + start.x + offset.x; // cast to avoid warnings
        const size_t len = size_t(end.x) - start.x + 1;
        for(size_t x = 0; x < len; ++x)
            if(psrc[x].a)
                pdst[x] = psrc[x];
    }
}

void tridraw(Image2d& out, ivec2 offset, const uvec2* points, const Tri* tris, size_t ntris, const Image2d& src)
{
    std::vector<uvec2> pointsWrk;
    pointsWrk.reserve(128); // guess
    for(size_t i = 0; i < ntris; ++i)
    {
        Tri t = tris[i];
        drawtri(out, offset, points[t.a], points[t.b], points[t.c], pointsWrk, src);
    }
}

size_t downsample4x4(Array2d<unsigned char>& out, Array2d<unsigned char>& in)
{
    assert(in.width() % 4 == 0 && in.height() % 4 == 0);
    const size_t ww = out.width();
    const size_t hh = out.height();
    assert(in.width() / 4 == ww && in.height() / 4 == hh);

    unsigned char *pout = out.data();
    size_t used = 0;

    for(size_t yy = 0; yy < hh; ++yy)
        for(size_t xx = 0; xx < ww; ++xx)
        {
            size_t blockused = 0;
            for(size_t y = 0; y < 4; ++y)
                for(size_t x = 0; x < 4; ++x)
                    blockused += in(x+xx*4, y+yy*4);

            out(xx, yy) = (unsigned char)blockused;
            used += !!blockused;
        }

    return used;
}

void triwireframe(Image2d& out, ivec2 offset, const uvec2* points, const Tri* tris, size_t ntris, Pixel color)
{
    std::set<unsigned> used;
    const uvec2 uoffset(offset);
    for(size_t i = 0; i < ntris; ++i)
    {
        const Tri& tri = tris[i];
        const unsigned AtoB = tri.a | (tri.b << 16u);
        const unsigned BtoA = tri.b | (tri.a << 16u);
        const unsigned BtoC = tri.b | (tri.c << 16u);
        const unsigned CtoB = tri.c | (tri.b << 16u);
        const unsigned CtoA = tri.c | (tri.a << 16u);
        const unsigned AtoC = tri.a | (tri.c << 16u);
        if(used.find(AtoB) == used.end() && used.find(BtoA) == used.end())
        {
            Line2dv line { points[tri.a] + uoffset, points[tri.b] + uoffset };
            line.intersect(LineDraw(out, color));
            used.insert(AtoB);
            used.insert(BtoA);
        }
        if(used.find(BtoC) == used.end() && used.find(CtoB) == used.end())
        {
            Line2dv line { points[tri.b] + uoffset, points[tri.c] + uoffset };
            line.intersect(LineDraw(out, color));
            used.insert(BtoC);
            used.insert(CtoB);
        }
        if(used.find(CtoA) == used.end() && used.find(AtoC) == used.end())
        {
            Line2dv line { points[tri.c] + uoffset, points[tri.a] + uoffset };
            line.intersect(LineDraw(out, color));
            used.insert(CtoA);
            used.insert(AtoC);
        }
    }
}
