#pragma once

#include <vector>
#include "util.h"

template<typename R, typename A, typename F>
size_t floodfill(R& ret, const A& a, size_t x, size_t y, F& f)
{
    struct Offs { int x, y; };
    static const Offs offsets[] = { {-1, 0}, { 1,0 }, {0, -1}, {0, 1} }; // (+) shape around center pixel, not including center

    struct Pos { size_t x, y; };
    std::vector<Pos> todo;

    size_t n = 0;
    Pos p { x, y };
    goto begin;
    do
    {
        p = todo.back();
        todo.pop_back();
begin:
        if(f(ret, a, p.x, p.y))
        {
            ++n;
            for(size_t i = 0; i < Countof(offsets); ++i)
            {
                Offs o = offsets[i];
                Pos pos { p.x+o.x, p.y+o.y };
                todo.push_back(pos);
            }
        }
    }
    while(!todo.empty());
    return n;
}

template<typename P, typename F>
bool linecast(const P& p0, const P& p1, F& f, int *pCollision)
{
    // Bresenham line tracing algorithm
    int x0 = (int)p0.x;
    int y0 = (int)p0.y;
    int x1 = (int)p1.x;
    int y1 = (int)p1.y;
    const int dx = std::abs(x1-x0);
    const int dy = std::abs(y1-y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    while(true)
    {
        if(f((size_t)x0, (size_t)y0))
        {
            if(pCollision)
            {
                pCollision[0] = x0;
                pCollision[1] = y0;
            }
            return true;
        }
        if(x0 == x1 && y0 == y1)
            break;
        int err2 = err * 2;
        if(err2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if(err2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
    return false;
}