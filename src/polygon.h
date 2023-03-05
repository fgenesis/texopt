#pragma once

#include <vector>
#include <cmath>
#include <assert.h>
#include "algo2d.h"

struct Point2d
{
    size_t x, y;

    inline bool operator==(const Point2d& o) const { return x == o.x && y == o.y; }
    inline bool operator!=(const Point2d& o) const { return !(*this == o); }
};

struct Line2d
{
    Point2d p0, p1;

    template<typename F>
    bool intersect(F& f) const
    {
        return linecast(p0, p1, f, NULL);
    }
};

// closed-loop polygon
struct Polygon
{
    std::vector<Point2d> points;

    Point2d getPoint(int i) const // translate out-of-bounds access to closed loop
    {
        int n = (int)points.size();
        if(i < 0)
            i += n;
        else if(i >= n)
            i -= n;
        return points[i];
    }

    // can remove point if the new line does not cut into the pixel grid (for convex corners).
    // In case of a concave corner, the point can always be removed, because connecting our
    // two neighbors can by definition never collide with the pixel grid
    template<typename F>
    bool canRemovePoint(int i, F& f) const
    {
        Line2d line { getPoint(i-1), getPoint(i+1) };
        return !line.intersect(f);
    }

    // return simplified polygon so that every line segment passes the collision check f(x,y)
    template<typename F>
    Polygon simplify(F& f) const
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
                if(reduced.points.size() > 1 && !linecast(anchor, reduced.points[1], f, NULL))
                    reduced.points[0] = anchor;
                else
                    reduced.points.push_back(anchor);
                break;
            }
            if(anchor != prev && linecast(anchor, next, f, NULL))
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

        std::vector<char> canremove(N);
        for(int i = 0; i < int(N); ++i)
            canremove[i] = canRemovePoint(i, f);
        
        size_t nsz = douglas_peucker(&reduced.points[0], N, &points[0], &canremove[0], N, epsilon);
        reduced.points.resize(nsz);

        return reduced;
    }

private:
    static size_t douglas_peucker(Point2d *pdst, size_t dstsize, const const Point2d *points, const char *canremove, size_t n, float epsilon)
    {
        assert(n >= 2);
        assert(epsilon >= 0);

        float max_dist = 0;
        size_t index = 0;
        size_t dstidx = 0;

        for (size_t i = 1; i + 1 < n; ++i)
        {
            if(!canremove[i])
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
            size_t n1 = douglas_peucker(pdst, dstsize, points, canremove, index + 1, epsilon);
            if (dstsize >= n1 - 1)
            {
                dstsize -= n1 - 1;
                pdst += n1 - 1;
            }
            else
                dstsize = 0;

            size_t n2 = douglas_peucker(pdst, dstsize, points + index, canremove + index, n - index, epsilon);
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
