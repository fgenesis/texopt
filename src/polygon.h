#pragma once

#include <vector>
#include <cmath>
#include <assert.h>
#include "algo2d.h"
#include "vec.h"
#include "image2d.h"


// TODO: This can go; make this uvec2
struct Point2d
{
    size_t x, y;

    inline bool operator==(const Point2d& o) const { return x == o.x && y == o.y; }
    inline bool operator!=(const Point2d& o) const { return !(*this == o); }

    Point2d to(const Point2d& dst) const
    {
        Point2d ret { dst.x - x, dst.y - y };
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

    bool triangulate(std::vector<Tri>& tris) const;

private:

    inline ivec2 _vec(int i) const
    {
        Point2d p = getPoint(i);
        ivec2 v { (int)p.x, (int)p.y };
        return v;
    }

    inline ivec2 _vec(size_t i) const
    {
        Point2d p = points[i];
        ivec2 v { (int)p.x, (int)p.y };
        return v;
    }

    bool isInternal(size_t originIdx, size_t prevIdx, size_t nextIdx) const;


    static size_t douglas_peucker(Point2d *pdst, size_t dstsize, const Point2d *points, const char *keep, size_t n, float epsilon);
};
