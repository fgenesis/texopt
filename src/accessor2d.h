#pragma once

#include "image2d.h"

template<typename A>
struct BoundsCheck
{
    BoundsCheck(const A& a) : w(a.width()), h(a.height()) {}
    const size_t w, h;
    bool inBounds(size_t x, size_t y) const { return x < w && y < h; }
    inline size_t width() const  { return w; }
    inline size_t height() const { return h; }
};

template<typename T>
struct GetValue : public BoundsCheck<Array2d<T> >
{
    GetValue(const Array2d<T>& img, T oob) : BoundsCheck(img), img(img), oob(oob) {}
    const Array2d<T>& img;
    const T oob;
    const T& operator()(size_t x, size_t y) const { return this->inBounds(x, y) ? img(x, y) : oob; }
};

template<typename R, typename A, typename F>
void generate(R& ret, const A& a, F& f)
{
    const size_t W = a.width(), H = a.height();
    ret.init(W, H);
    for(size_t y = 0; y < H; ++y)
        for(size_t x = 0; x < W; ++x)
            ret(x, y) = f(a(x, y));
}

template<typename R, typename A, typename F>
void generate2(R& ret, const A& a, F& f)
{
    const size_t W = a.width(), H = a.height();
    ret.init(W, H);
    for(size_t y = 0; y < H; ++y)
        for(size_t x = 0; x < W; ++x)
            ret(x, y) = f(a, x, y);
}
