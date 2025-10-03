#pragma once

#include <vector>
#include <cmath>
#include <assert.h>
#include "algo2d.h"
#include "vec.h"
#include "image2d.h"

enum PointFlags
{
    PT_CONVEX = 0x01
};

struct Point2d
{
    size_t x, y;

    union
    {
        PointFlags flags;
        unsigned idx;
    } u;

    inline bool operator==(const Point2d& o) const { return x == o.x && y == o.y; }
    inline bool operator!=(const Point2d& o) const { return !(*this == o); }

    Point2d to(const Point2d& dst) const
    {
        Point2d ret { dst.x - x, dst.y - y, u };
        return ret;
    }
};


template<typename P>
struct Line2dT
{
    P p0, p1;

    template<typename F>
    bool intersect(F& f) const
    {
        return linecast(p0, p1, f, NULL);
    }
};

typedef Line2dT<Point2d> Line2d;
typedef Line2dT<uvec2> Line2dv;

struct Tri
{
    size_t a, b, c;
};

// closed-loop polygon
struct Polygon
{
    static size_t VertexPenalty;
    std::vector<Point2d> points;

    Point2d getPoint(int i) const; // translate out-of-bounds access to closed loop

    size_t calcArea() const;

    size_t calcScore() const; // lower is better

    AABB getBoundingRect() const;


    // can remove point if the new line does not cut into the pixel grid (for convex corners).
    // In case of a concave corner, the point can always be removed, because connecting our
    // two neighbors can by definition never collide with the pixel grid
    template<typename F>
    bool intersectSolid(int i, F& f) const
    {
        Line2d line { getPoint(i-1), getPoint(i+1) };
        return line.intersect(f);
    }

    // return simplified polygon so that every line segment passes the collision check f(x,y)
    template<typename F>
    Polygon simplify(F& collision, const float maxdist) const
    {
        Polygon reduced;

        int i = 0;
        const Point2d first = getPoint(i++);
        Point2d anchor = first;
        Point2d prev = first;
        const int N = points.size();

        for(;;)
        {
            Point2d next = getPoint(i);
            if(next == first)
            {
                // if we can see points[1] from here, the first point is unnecessary
                if(reduced.points.size() > 1 && !linecast(anchor, reduced.points[1], collision, NULL))
                    reduced.points[0] = anchor;
                else
                    reduced.points.push_back(anchor);
                break;
            }
            if(anchor != prev && ((maxdist > 0 && point_distance<float>(anchor, next) > maxdist) || linecast(anchor, next, collision, NULL)))
            {
                reduced.points.push_back(anchor);
                anchor = prev; // continue from here
                continue;
            }
            prev = next; // this point is not colliding
            ++i;
        }

        return reduced;
    }

    template<typename F>
    Polygon simplifyDP(F& f, float epsilon) const
    {
        // modified Douglas-Peucker polyline simplification algorithm
        Polygon reduced;
        const size_t N = points.size();
        reduced.points.resize(N);

        std::vector<char> keep(N);
        bool solid = false;
        for(int i = 0; i < int(N); ++i)
        {
            const bool solidnow = intersectSolid(i, f);
            keep[i] = solidnow != solid;
            solid = solidnow;
        }
        size_t nsz = douglas_peucker(&reduced.points[0], N, &points[0], &keep[0], N, epsilon);
        reduced.points.resize(nsz);

        return reduced;
    }

    bool triangulate_badly(std::vector<Tri>& tris) const;
    bool triangulate(std::vector<Tri>& tris) const;


#if 0
    bool isConvexPoint(size_t id) const
    {
        Point2d prev = getPoint(id - 1);
        Point2d p = getPoint(id);
        Point2d next = getPoint(id + 1);

        Point2d dprev = p.to(prev);
        Point2d dnext = p.to(next);

        /*float vax = (float)dprev.x;
        float vay = (float)dprev.y;
        float vbx = (float)dnext.x;
        float vby = (float)dnext.y;

        float alen = std::sqrtf(vax*vax + vay*vay);
        float blen = std::sqrtf(vbx*vbx + vby*vby);

        vax /= alen;
        vay /= alen;
        vbx /= blen;
        vby /= blen;*/

        Point2d d = dprev.to(dnext);
        float dx = float(d.x);
        float dy = float(d.y);

        float a = std::atan2f(dx, std::fabsf(dx));
        a = (a/3.1415962f)*180;
        if (dx < 0)
            a = 180-a;
        a += 90;
        return a >= 180;
    }

    bool isClockwise() const
    {
        Point2d bottomRight = points[0];
        size_t bottomRightIndex = 0;

        const size_t N = points.size();
        for (size_t i = 0; i < N; i++)
        {
            const Point2d p = points[i];
            const bool check = p.y != bottomRight.y ? p.y < bottomRight.y : p.x > bottomRight.x;
            if (check)
            {
                bottomRight = p;
                bottomRightIndex = i;
            }
        }

        const size_t prevIdx = (N + bottomRightIndex - 1) % N;
        const size_t nextIdx = (bottomRightIndex + 1) % N;

        return !isInternal(bottomRightIndex, prevIdx, nextIdx);
    }

    void triangulate(std::vector<Tri>& result) const
    {
        size_t n = points.size();
        if(n < 3)
            return;
        std::vector<Point2d> pp = points;
        std::vector<size_t> ear;
        for (size_t i = 0; i < n; i++)
        {
            pp[i].u.idx = (unsigned)i;

            size_t prev = (i + n - 1) % n;
            size_t next = (i + 1) % n;
            for (size_t j = 0; j < n; j++)
            {
                if (j == i || j == prev || j == next)
                    continue;
                if (cross(pp[prev], pp[i], pp[j]) >= 0
                 && cross(pp[i], pp[next], pp[j]) >= 0
                 && cross(pp[next], pp[prev], pp[j]) >= 0
                )
                    goto skip;
            }
            ear.push_back(i);
            skip: ;
        }
        while (!ear.empty())
        {
            const size_t i = ear.back();
            ear.pop_back();
            const size_t prev = (i + n - 1) % n;
            const size_t next = (i + 1) % n;
            Tri tri { pp[prev].u.idx, pp[i].u.idx, pp[next].u.idx }; // save indices in the original polygon
            result.push_back(tri);
            pp.erase(pp.begin() + i);
            --n;
            const size_t prev_prev = (prev + n - 1) % n;
            const size_t next_next = (next + 1) % n;
            if (prev != next_next)
                for (size_t j = 0; j < n; j++)
                {
                    if (j == prev || j == next_next)
                        continue;
                    if (cross(pp[prev], pp[next_next], pp[j]) < 0)
                        ear.push_back(prev);
                }
            if (prev_prev != next)
                for (int j = 0; j < n; j++) {
                    if (j == prev_prev || j == next)
                        continue;
                    if (cross(pp[prev_prev], pp[next], pp[j]) < 0)
                        ear.push_back(next);
                }
        }
    }
#endif

private:

    ivec2 _vec(int i) const
    {
        Point2d p = getPoint(i);
        ivec2 v { (int)p.x, (int)p.y };
        return v;
    }

    ivec2 _vec(size_t i) const
    {
        Point2d p = points[i];
        ivec2 v { (int)p.x, (int)p.y };
        return v;
    }

    bool isInternal(size_t originIdx, size_t prevIdx, size_t nextIdx) const
    {
        ivec2 origin = _vec(originIdx);
        ivec2 next = _vec(nextIdx);
        ivec2 prev = _vec(prevIdx);
        ivec2 localPrev = prev - origin;
        ivec2 localNext = next - origin;
        return cross(localNext, localPrev) > 0;
    }

    static size_t douglas_peucker(Point2d *pdst, size_t dstsize, const Point2d *points, const char *keep, size_t n, float epsilon)
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
};
