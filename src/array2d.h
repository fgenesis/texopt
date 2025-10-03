#pragma once

#include <stddef.h>
#include <vector>
#include <algorithm>
#include <assert.h>

class Array2dAny
{
protected:
    size_t _w, _h;

    Array2dAny() : _w(0), _h(0) {}
    Array2dAny(size_t w, size_t h) : _w(w), _h(h) {}

public:
    inline size_t width() const {return _w;}
    inline size_t height() const {return _h;}
};

template<typename T>
class Array2d : public Array2dAny
{
protected:
    std::vector<T> _v;

public:
    Array2d() : Array2dAny() {}
    Array2d(size_t w, size_t h) : Array2dAny(w, h), _v(w*h) {}


    void init(size_t w, size_t h)
    {
        _w = w;
        _h = h;
        _v.resize(w*h);
    }

    void clear()
    {
        _w = _h = 0;
        _v.clear();
    }

    void fill(const T& v)
    {
        std::fill(_v.begin(), _v.end(), v);
    }

    void copy2d(size_t dstx, size_t dsty, const Array2d<T>& src, size_t srcx, size_t srcy, size_t w, size_t h)
    {
        assert(dstx + w <= width());
        assert(dsty + h <= height());
        assert(srcx + w <= src.width());
        assert(srcy + h <= src.height());

        for(size_t y = 0; y < h; ++y)
        {
            T *dstrow = row(dsty + y);
            const T *srcrow = src.row(srcy + y);
            std::copy(srcrow + srcx, srcrow + srcx + w, dstrow + dstx);
        }
    }

    void resize(size_t w, size_t h)
    {
        if(_w && _h)
        {
            Array2d<T> cp;
            cp.swap(*this);
            init(w, h);
            size_t cpw = std::min(w, cp.width());
            size_t cph = std::min(h, cp.height());
            copy2d(0, 0, cp, 0, 0, cpw, cph);
        }
        else
            init(w, h);
    }

    void swap(Array2d<T>& other)
    {
        _v.swap(other._v);
        std::swap(_w, other._w);
        std::swap(_h, other._h);
    }


    inline T& operator()(size_t x, size_t y)
    {
        return _v[y * _w + x];
    }
    inline const T& operator()(size_t x, size_t y) const
    {
        return _v[y * _w + x];
    }

    const T *data() const { return _v.empty() ? NULL : &_v[0]; }
          T *data()       { return _v.empty() ? NULL : &_v[0]; }

    const T *row(size_t y) const { return &_v[y * _w]; }
          T *row(size_t y)       { return &_v[y * _w]; }

    void fh()
    {
        const size_t w = width();
        const size_t h = height();
        for(size_t y = 0; y < h; ++y)
        {
            T *begin = row(y);
            T *end = begin + w;
            while(begin < end)
            {
                std::swap(*begin, *end);
                ++begin;
                --end;
            }
        }
    }
};
