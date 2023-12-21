#include "polygon.h"

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

bool Polygon::triangulate(std::vector<Tri>& results) const
{
    const size_t N = points.size();

    if(points.size() < 3)
        return false;

    // Polygon orientation
    bool ccw = false;
    {
        Point2d left = points[0];
        size_t index = 0;

        for(size_t i = 0; i < N; ++i)
        {
            if(points[i].x < left.x ||
              (points[i].x == left.x && points[i].y < left.y))
            {
                index = i;
                left = points[i];
            }
        }

        Point2d tri[]
        {
            points[(index > 0) ? index - 1 : N - 1],
            points[index],
            points[(index < N) ? index + 1 : 0]
        };
        ccw = orientation(tri[0], tri[1], tri[2]);
    }

    // We know there will be vertex_count - 2 triangles made.
    results.reserve(results.size() + N - 2);

    // store indices in original, unmodified polygon
    std::vector<Point2d> pp = points;
    for(size_t i = 0; i < N; ++i)
        pp[i].u.idx = i;

    std::vector<size_t> reflex;
    for(;;)
    {
        const size_t n = pp.size();
        if(n < 3)
            break;
        reflex.clear();
        int eartip = -1, index = -1;
        for(size_t i = 0; i < pp.size(); ++i)
        {
            ++index;
            if(eartip >= 0) break;

            size_t p = (index > 0) ? index - 1 : n - 1;
            size_t q = (index < n) ? index + 1 : 0;

            Point2d tri[] = { pp[p], pp[i], pp[q] };
            if(orientation(tri[0], tri[1], tri[2]) != ccw)
            {
                reflex.push_back(index);
                continue;
            }

            bool ear = true;
            for(size_t ii = 0; ii < reflex.size(); ++ii)
            {
                size_t j = reflex[ii];
                if(j == p || j == q) continue;
                if(in_triangle(pp[j], pp[p], pp[i], pp[q]))
                {
                    ear = false;
                    break;
                }
            }

            if(ear)
            {
                std::vector<Point2d>::iterator
                    j = pp.begin() + index + 1,
                    k = pp.end();

                for( ; j != k; ++j)
                {
                    const Point2d& v = *j;

                    if(&v == &pp[p] ||
                       &v == &pp[q] ||
                       &v == &pp[index]) continue;

                    if(in_triangle(v, pp[p], pp[i], pp[q]))
                    {
                        ear = false;
                        break;
                    }
                }
            }

            if(ear)
                eartip = index;
        }

        if(eartip < 0)
            break;

        size_t p = (eartip > 0) ? eartip - 1 : n - 1;
        size_t q = (eartip < n) ? eartip + 1 : 0;

        // Create the triangulated piece.
        Tri out = { pp[p].u.idx, pp[eartip].u.idx, pp[q].u.idx };
        results.push_back(out);

        // Clip the ear from the polygon.
        pp.erase(pp.begin() + eartip);
    }

    return true;
}
