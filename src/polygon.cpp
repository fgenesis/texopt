#include "polygon.h"
#include "polypartition.h"

size_t Polygon::VertexPenalty = 1024;

// via:
// https://abitwise.blogspot.com/2013/09/triangulating-concave-and-convex.html
// https://gist.github.com/Shaptic/6526805
static bool orientation(const Point2d& a, const Point2d& b, const Point2d& c)
{
    return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y) > 0;
}

// Barycentric coordinate calculation.
static bool in_triangle(const Point2d& V, const Point2d& A,
                    const Point2d& B, const Point2d& C)
{
    size_t q = ((B.y - C.y) * (A.x - C.x) + (C.x - B.x) * (A.y - C.y));
    if(!q)
        return true;
    float denom = 1.0f / q;

    float alpha = denom * ((B.y - C.y) * (V.x - C.x) + (C.x - B.x) * (V.y - C.y));
    if(alpha < 0) return false;

    float beta  = denom * ((C.y - A.y) * (V.x - C.x) + (A.x - C.x) * (V.y - C.y));
    if(beta < 0) return false;

    return alpha + beta >= 1;
}

size_t Polygon::calcScore() const
{
    size_t area = calcArea(); // smaller is better

    size_t tris = points.size(); // FIXME: actually use #tris

    // TODO: include relative size too
    return tris * VertexPenalty + area;
}

AABB Polygon::getBoundingRect() const
{
    AABB box = {size_t(-1),size_t(-1),0,0};

    const size_t N = points.size();

    for(size_t i = 0; i < N; ++i)
    {
        const Point2d& p = points[i];
        box.x1 = std::min(box.x1, p.x);
        box.y1 = std::min(box.y1, p.y);
        box.x2 = std::max(box.x2, p.x);
        box.y2 = std::max(box.y2, p.y);
    }

    return box;
}

bool Polygon::triangulate(std::vector<Tri>& results) const
{
    TPPLPoly tp;
    tp.Init(points.size());
    //for(size_t i = points.size(); i --> 0; )
    for(size_t i = 0; i < points.size(); ++i)
    {
        tp[i].x = points[i].x;
        tp[i].y = points[i].y;
        tp[i].id = i;
    }

    TPPLPolyList tris;
    TPPLPartition pp;
    if(!pp.Triangulate_OPT(&tp, &tris))
        return false;

    results.reserve(results.size() + tris.size());

    for(TPPLPolyList::iterator it = tris.begin(); it != tris.end(); ++it)
    {
        TPPLPoly& o = *it;
        assert(o.GetNumPoints() == 3);
        Tri t;
        t.a = o[0].id;
        t.b = o[1].id;
        t.c = o[2].id;
        results.push_back(t);
    }

    return true;
}

bool Polygon::isInternal(size_t originIdx, size_t prevIdx, size_t nextIdx) const
{
    ivec2 origin = _vec(originIdx);
    ivec2 next = _vec(nextIdx);
    ivec2 prev = _vec(prevIdx);
    ivec2 localPrev = prev - origin;
    ivec2 localNext = next - origin;
    return cross(localNext, localPrev) > 0;
}

size_t Polygon::douglas_peucker(Point2d* pdst, size_t dstsize, const Point2d* points, const char* keep, size_t n, float epsilon)
{
    assert(n >= 2);
    assert(epsilon >= 0);

    float max_dist = 0;
    size_t index = 0;
    size_t dstidx = 0;

    for (size_t i = 1; i + 1 < n; ++i)
    {
        if(keep[i])
            continue;

        float dist = perpendicular_distance<float>(points[i], points[0], points[n - 1]);
        if (dist > max_dist)
        {
            max_dist = dist;
            index = i;
        }
    }

    if (max_dist > epsilon)
    {
        size_t n1 = douglas_peucker(pdst, dstsize, points, keep, index + 1, epsilon);
        if (dstsize >= n1 - 1)
        {
            dstsize -= n1 - 1;
            pdst += n1 - 1;
        }
        else
            dstsize = 0;

        size_t n2 = douglas_peucker(pdst, dstsize, points + index, keep + index, n - index, epsilon);
        return n1 + n2 - 1;
    }
    if (dstsize >= 2)
    {
        pdst[0] = points[0];
        pdst[1] = points[n - 1];
    }
    return 2;
}

Point2d Polygon::getPoint(int i) const // translate out-of-bounds access to closed loop
{
    int n = (int)points.size();
    if(i < 0)
        i += n;
    else if(i >= n)
        i -= n;
    return points[i];
}

size_t Polygon::calcArea() const
{
    int area = 0;
    const size_t N = points.size();
    size_t j = N-1;

    for (size_t i = 0; i < N; ++i)
    {
        const Point2d& pi = points[i];
        const Point2d& pj = points[j];
        area += (int(pj.x)+int(pi.x)) * (int(pj.y)-int(pi.y));
        j = i;
    }
    return (size_t)std::abs(area / 2);
}

