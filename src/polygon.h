#pragma once

#include <vector>
#include <cmath>
#include <assert.h>
#include "algo2d.h"

enum PointFlags
{
    PT_CONVEX = 0x01
};

struct Point2d
{
    size_t x, y;
    PointFlags flags;

    inline bool operator==(const Point2d& o) const { return x == o.x && y == o.y; }
    inline bool operator!=(const Point2d& o) const { return !(*this == o); }

    Point2d to(const Point2d& dst) const
    {
        Point2d ret { dst.x - x, dst.y - y, flags };
        return ret;
    }
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

    size_t calcArea() const
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
        for(int i = 0; i < int(N); ++i)
            keep[i] = intersectSolid(i, f);

        size_t nsz = douglas_peucker(&reduced.points[0], N, &points[0], &keep[0], N, epsilon);
        reduced.points.resize(nsz);

        return reduced;
    }

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

private:
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
