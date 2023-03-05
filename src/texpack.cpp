#include "accessor2d.h"
#include "algo2d.h"
#include "util.h"
#include "polygon.h"
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning(disable:26812) // unscoped enum
#endif

enum PixelFlag
{
    PF_EMPTY = 0,
    PF_SOLID = 0x01, // actually solid pixels (= anything that is not fully transparent)
    PF_DILATED = 0x02, // near solid pixels
    PF_BOUNDARY = 0x04,
};
PixelFlag operator|(PixelFlag a, PixelFlag b) { return PixelFlag(size_t(a) | size_t(b)); }
PixelFlag& operator|=(PixelFlag& a, PixelFlag b) { return (a = a | b); }


typedef Array2d<PixelFlag> Flags2d;

static const Pixel TransparentPixel { 0,0,0,0 };
static const Pixel BlackPixel { 0,0,0,255 };
static const Pixel WhitePixel { 255,255,255,255 };
static const Pixel RedPixel { 255,0,0,255 };
static const Pixel GreyPixel { 127,127,127,255 };

static PixelFlag isNotFullyTransparent(Pixel p)
{
    return p.a ? PF_SOLID : PF_EMPTY;
}

static PixelFlag addBoundaryFlag(const GetValue<PixelFlag>& a, size_t x, size_t y)
{
    struct Offs { int x, y; };
    static const Offs offsets[] = { {-1, 0}, { 1,0 }, {0, -1}, {0, 1} }; // (+) shape around center pixel, not including center
    PixelFlag here = a(x, y);
    if(here & (PF_SOLID|PF_DILATED))
    {
        size_t solid = 0;
        size_t empty = 0;
        for(size_t i = 0; i < Countof(offsets); ++i)
        {
            Offs o = offsets[i];
            if(a(x+o.x, y+o.y) & (PF_SOLID|PF_DILATED))
                ++solid;
            else
                ++empty;
        }
        if(empty && solid)
            here |= PF_BOUNDARY;
    }
    return here;
}

static PixelFlag addDilatedFlag(const GetValue<PixelFlag>& a, size_t x, size_t y)
{
    PixelFlag here = a(x, y);
    if(!(here & (PF_DILATED|PF_SOLID)))
    {
        for(int yy = -1; yy <= 1; ++yy)
            for(int xx = -1; xx <= 1; ++xx)
                if(a(x+xx,y+yy) & (PF_SOLID | PF_DILATED))
                    return here | PF_DILATED;
    }
    return here;
}

struct ShowBit
{
    ShowBit(unsigned bit) : bit(bit) {}
    const unsigned bit;
    Pixel operator()(unsigned x) const { return x & bit ? WhitePixel : GreyPixel; }
};

struct MetaPixel
{
    PixelFlag flag;
    unsigned cc;
};
typedef Array2d<MetaPixel> Meta2d;

inline static MetaPixel initMetaPixel(PixelFlag f)
{
    MetaPixel p{ f, 0 };
    return p;
}

struct Filler
{
    Filler(unsigned cc) : cc(cc) {}
    const unsigned cc;

    template<typename A>
    inline bool operator()(Meta2d& dst, const A& a, size_t x, size_t y)
    {
        MetaPixel src = a(x,y);
        if(src.cc || !(src.flag & (PF_SOLID|PF_DILATED)))
            return false;

        dst(x,y).cc = cc;
        return true;
    }
};

struct RegionAnnotator
{
    Meta2d& tofill;
    unsigned cc;

    RegionAnnotator(Meta2d& tofill) : tofill(tofill), cc(0) {}

    template<typename A>
    MetaPixel operator()(const A& a, size_t x, size_t y)
    {
        int dummy = 0;
        size_t filled = floodfill(tofill, a, x, y, Filler(cc + 1));
        if(filled)
            ++cc;

        return a(x,y);
    };
};

struct ShowBitAndCC
{
    ShowBitAndCC(unsigned bit) : bit(bit) {}
    const unsigned bit;
    Pixel operator()(const MetaPixel& mp) const
    {
        Pixel p;
        unsigned val = mp.cc * 37;
        p.g = val & 0xff; val >>= 8;
        p.r = val & 0xff; val >>= 8;
        p.b = val & 0xff; val >>= 8;
        p.a = 0xff;
        if(mp.flag & bit)
        {
            p.r = 0xff;
            p.b = 0xff;
        }
        return p;
    }
};

static void distributeConnectedRegions(Meta2d& meta, const Flags2d& solid)
{
    MetaPixel oob { PF_EMPTY, unsigned(-1) };
    generate(meta, solid, initMetaPixel);
    RegionAnnotator ra(meta);
    generate2(meta, GetValue<MetaPixel>(meta, oob), ra);
    printf("CC: %u\n", ra.cc);
}

static void generatePolygon(Polygon& poly, Array2d<char>& used, const GetValue<MetaPixel>& get, size_t x, size_t y, unsigned cc)
{
    struct Offs { int x, y; };
    static const Offs offsets[] =
    {
        {-1, 0}, { 1, 0 }, {0, -1}, {0, 1}, // prefer going in a straight line (IMPORTANT to create correct polygons around corners)
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}  // and only go diagonally if straight won't work
    };


    const Point2d orig { x, y };
    poly.points.push_back(orig);
    used(orig.x, orig.y) = 1;

nextpoint:
    for(size_t i = 0; i < Countof(offsets); ++i)
    {
        Offs o = offsets[i];
        Point2d p { x+o.x, y+o.y };
        MetaPixel mp = get(p.x, p.y);
        if((mp.flag & PF_BOUNDARY) && mp.cc == cc && !used(p.x, p.y))
        {
            //printf("Moved from (%u, %u) by (%d, %d) to (%u, %u)\n", unsigned(x), unsigned(y), xx, yy, unsigned(p.x), unsigned(p.y));
            used(p.x, p.y) = 1;
            poly.points.push_back(p);
            x = p.x;
            y = p.y;
            goto nextpoint;
        }
    }

    // no neighbor taken? must have reached start -> closed loop is complete

    //Point2d last = poly.points.back();
    //assert(orig != last && std::abs(orig.x - last.x) <= 1 && std::abs(orig.y - last.y));
}

static std::vector<Polygon> generatePolygons(const Meta2d& meta)
{
    std::vector<Polygon> polys;
    const MetaPixel oob { PF_EMPTY, unsigned(-1) };
    const GetValue<MetaPixel> get(meta, oob);
    Array2d<char> used(meta.width(), meta.height());
    used.fill(0);
    std::vector<unsigned> done;
    const size_t W = meta.width(), H = meta.height();
    for(size_t y = 0; y < H; ++y)
        for(size_t x = 0; x < W; ++x)
        {
            MetaPixel p = meta(x,y);
            // Because we go through the image line by line, we will never hit an internal boundary (if there are holes) before hitting an external one
            // And since we then follow along that boundary and mark that CC as used, the internal boundary will never be used to generate a polygon
            if(p.flag & PF_BOUNDARY && !used(x,y))
                if(const unsigned cc = p.cc)
                {
                    if(done.size() < cc)
                        done.resize(cc, 0);
                    const unsigned idx = cc - 1;
                    if(!done[idx])
                    {
                        done[idx] = 1;
                        if(polys.size() < cc)
                            polys.resize(cc);
                        generatePolygon(polys[idx], used, get, x, y, cc);
                        printf("Generated polygon with %u points for CC %u\n", (unsigned)polys[idx].points.size(), cc);
                    }
                }
        }
    return polys;
}

struct CollidesWithPixels
{
    const Flags2d& solid;
    CollidesWithPixels(const Flags2d& solid) : solid(solid) {}

    inline bool operator()(size_t x, size_t y)
    {
        return solid(x,y) & PF_SOLID;
    }

};

struct EmbossLine
{
    Image2d& img;
    const Pixel pix;
    EmbossLine(Image2d& img, Pixel pix) : img(img), pix(pix) {}
    bool operator()(size_t x, size_t y)
    {
        img(x,y) = pix;
        return false; // never collide -> always draw the full line
    }
};

enum { DILATION_ROUNDS = 3 };

bool doOneImage(const char *fn)
{
    Image2d img;
    if(!img.load(fn))
    {
        printf("Failed to load image: %s\n", fn);
        return false;
    }

    Image2d out;

    Flags2d solid;
    generate(solid, img, isNotFullyTransparent);
    {
        Flags2d tmp; // can't dilate into the source buffer because dilation checks for the same flag it sets...
        for(unsigned i = 0; i < DILATION_ROUNDS; ++i)
        {
            tmp.swap(solid); // ... so it's buffer ping-pong then.
            generate2(solid, GetValue<PixelFlag>(tmp, PF_EMPTY), addDilatedFlag);
        }
    }
    generate2(solid, GetValue<PixelFlag>(solid, PF_EMPTY), addBoundaryFlag);

    generate(out, solid, ShowBit(PF_BOUNDARY));
    out.writePNG("boundary.png");
    generate(out, solid, ShowBit(PF_DILATED));
    out.writePNG("dilated.png");

    Meta2d meta;
    distributeConnectedRegions(meta, solid);

    generate(out, meta, ShowBitAndCC(PF_BOUNDARY));
    out.writePNG("cc.png");

    std::vector<Polygon> polys = generatePolygons(meta);
    std::vector<Polygon> simplepolys;
    simplepolys.reserve(polys.size());
    for(size_t i = 0; i < polys.size(); ++i)
    {
        //simplepolys.push_back(polys[i].simplify(CollidesWithPixels(solid)));
        simplepolys.push_back(polys[i].simplifyDP(CollidesWithPixels(solid), 5));
        const Polygon& last = simplepolys.back();
        printf("Simplified polygon[%u] from %u to %u points\n", unsigned(i), (unsigned)polys[i].points.size(), (unsigned)last.points.size());
    }

    out = img;
    bool flop = false;
    for(size_t i = 0; i < simplepolys.size(); ++i)
    {
        const Polygon& poly = simplepolys[i];
        for(int k = 0; k < (int)poly.points.size(); ++k)
        {
            Line2d line { poly.getPoint(k), poly.getPoint(k+1) };
            line.intersect(EmbossLine(out, flop ? WhitePixel : RedPixel));
            flop = !flop;
        }
    }
    out.writePNG("embossed.png");

    return true;
}


int main(int argc, char *argv[])
{
    //doOneImage("gear.png");
    doOneImage("face.png");
    //doOneImage("menu-frame2.png");
    //doOneImage("pyramid-dragon-bg.png");
    //doOneImage("tentacles.png");
    //doOneImage("rock0002.png");
    //doOneImage("sunstatue-0001.png");
    //doOneImage("city-stairs.png");


    printf("Exiting.\n");
    return 0;
}
