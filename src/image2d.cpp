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


bool Image2d::writePNG(const char* fn)
{
    const size_t N = width() * height();
    stbi_uc * const d = (stbi_uc*)malloc(N * 4);
    if(!d)
        return false;
    stbi_uc *p = d;

    for(size_t i = 0; i < N; ++i)
    {
        Pixel c = _v[i];
        *p++ = c.r;
        *p++ = c.g;
        *p++ = c.b;
        *p++ = c.a;
    }

    int ok = stbi_write_png(fn, (int)width(), (int)height(), 4, d, 0);
    free(d);
    return !!ok;
}


bool Image2d::load(const char* fn)
{
    int x, y, c;
    stbi_uc * const d = stbi_load(fn, &x, &y, &c, 4);
    if(!d)
        return false;

    this->init(x, y);
    const size_t N = size_t(x) * size_t(y);
    stbi_uc *p = d;
    for(size_t i = 0; i < N; ++i)
    {
        Pixel pix;
        pix.r = *p++;
        pix.g = *p++;
        pix.b = *p++;
        pix.a = *p++;
        _v[i] = pix;

    }
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
    stbir_resize_uint8(
        (const unsigned char*)src.data(), src.width(), src.height(), 0,
        (unsigned char*)data(), _w, _h, 0, 4);
}
