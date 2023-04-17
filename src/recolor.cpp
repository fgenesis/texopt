#include <stdio.h>
#include "image2d.h"

struct Color
{
    unsigned r, g, b;
};
struct Colorf
{
    float r, g, b;
};

static Pixel conv(Pixel in, Colorf c)
{
    Pixel out =
    {
        in.r * c.r,
        in.g * c.g,
        in.b * c.b,
        in.a
    };
    return out;
}

const Color colors[] =
{
    { 0x0A, 0xC1, 0x00 },
    { 0x00, 0xE2, 0xCF },
    { 0x00, 0x61, 0xF2 },
    { 0x5E, 0x00, 0xBE },
    { 0xF4, 0x00, 0xD6 },
    { 0xF8, 0x00, 0x00 },
    { 0xFF, 0x7A, 0x0D },
    { 0xFF, 0xD6, 0x2F }
};

int main(int argc, char *argv[])
{
    char fn[256];

    for(unsigned f = 0; f < 8; ++f)
    {
        const Color& c = colors[f];
        sprintf(fn, "notesymbol%u.png", f);
        Image2d img;
        img.load(fn);

        const size_t N = img.width() * img.height();
        Pixel *p = img.data();
        Colorf cf =
        {
            float(c.r) / 255.f,
            float(c.g) / 255.f,
            float(c.b) / 255.f
        };
        for(size_t i = 0; i < N; ++i)
            p[i] = conv(p[i], cf);

        sprintf(fn, "_%u.png", f);
        img.writePNG(fn);

    }
}

