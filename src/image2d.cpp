#include "image2d.h"
#include "image2d.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_resize.h"
#include <stdlib.h>
#include <string.h>

Image2d::Image2d()
{
}

Image2d::Image2d(size_t w, size_t h)
: Array2d(w, h)
{
}


bool Image2d::writePNG(const char* fn) const
{
    const size_t N = width() * height();
    return N && stbi_write_png(fn, (int)width(), (int)height(), 4, (stbi_uc*)&_v[0], 0);
}


bool Image2d::load(const char* fn)
{
    int x, y, c;
    stbi_uc * const d = stbi_load(fn, &x, &y, &c, 4);
    if(!d || !x || !y)
        return false;

    this->init(x, y);
    memcpy(&_v[0], d, size_t(x) * size_t(y) * sizeof(Pixel));

    free(d);
    return true;
}

AABB Image2d::getAlphaRegion() const
{
    AABB ext { _w, _h, 0, 0 };
    const Pixel *pp = data();
    for(size_t y = 0; y < _h; ++y)
        for(size_t x = 0; x < _w; ++x)
        {
            const Pixel p = *pp++;
            if(p.a)
            {
                if(x < ext.x1)
                    ext.x1 = x;
                if(y < ext.y1)
                    ext.y1 = y;
                if(x > ext.x2)
                    ext.x2 = x;
                if(y > ext.y2)
                    ext.y2 = y;
            }
        }

    return ext;
}

void Image2d::copyscaled(const Image2d& src)
{
    stbir_resize_uint8_generic(
        (const unsigned char*)src.data(), src.width(), src.height(), 0,
        (unsigned char*)data(), _w, _h, 0, 4,
        3, 0, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, STBIR_COLORSPACE_LINEAR, NULL
    );
}

void Image2d::maskblit(const Image2d& top)
{
    assert(width() == top.width() && height() == top.height());
    const size_t n = width() * height();
    Pixel *dst = data();
    const Pixel *p = top.data();
    for(size_t i = 0; i < n; ++i, ++dst, ++p)
        if(p->a)
            *dst = *p;
}

Image2d& Image2d::operator=(const Image2d& o)
{
    this->init(o.width(), o.height());
    this->copy2d(0, 0, o, 0, 0, o.width(), o.height());
    return *this;
}
