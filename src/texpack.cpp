#include "accessor2d.h"
#include "algo2d.h"
#include "util.h"
#include "polygon.h"
#include "vertexbuf.h"
#include <stdio.h>

#ifdef _MSC_VER
#pragma warning(disable:26812) // unscoped enum
#endif

enum PixelFlag
{
    PF_EMPTY = 0
   ,PF_SOLID = 0x01 // actually solid pixels (= anything that is not fully transparent)
   ,PF_DILATED = 0x02 // near solid pixels
   ,PF_BOUNDARY = 0x04 // after dilating, this is the boundary to the outside
   ,PF_POLYGONBAND = 0x08 // polygon lines are only allowed in areas with this bit set
   ,PF_NO_HOLE = 0x10 // definitely not a hole
   ,PF_USED = 0x20 // covered by polygon
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

static PixelFlag closeHoles(const GetValue<PixelFlag>& a, size_t x, size_t y)
{
    PixelFlag here = a(x, y);
    if(!(here & (PF_DILATED|PF_SOLID|PF_NO_HOLE)))
        here |= PF_DILATED;
    return here;
}

static PixelFlag addPolygonFlag(const GetValue<PixelFlag>& a, size_t x, size_t y)
{
    PixelFlag here = a(x, y);
    if(here & PF_SOLID)
        return here; // can never construct polygon crossing solid area

    if(here & PF_DILATED)
        return here | PF_POLYGONBAND; // already dilated

    for(int yy = -1; yy <= 1; ++yy)
        for(int xx = -1; xx <= 1; ++xx)
            if(a(x+xx,y+yy) & (PF_POLYGONBAND|PF_SOLID|PF_DILATED))
                return here | PF_POLYGONBAND;

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

struct RegionAnnotator
{
    Meta2d& tofill;
    unsigned cc;

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


    RegionAnnotator(Meta2d& tofill) : tofill(tofill), cc(0) {}

    template<typename A>
    MetaPixel operator()(const A& a, size_t x, size_t y)
    {
        size_t filled = floodfill(tofill, a, x, y, Filler(cc + 1));
        if(filled)
            ++cc;

        return a(x,y);
    };
};

struct NoHoleAnnotator
{
    Flags2d& tofill;

    struct Filler
    {
        template<typename A>
        inline bool operator()(Flags2d& dst, const A& a, size_t x, size_t y)
        {
            PixelFlag src = a(x,y);
            if(src & (PF_NO_HOLE|PF_SOLID|PF_DILATED))
                return false; // already annotated or solid. solid is never a hole

            for(int yy = -1; yy <= 1; ++yy)
                for(int xx = -1; xx <= 1; ++xx)
                    if(a(x+xx,y+yy) & PF_NO_HOLE)
                    {
                        dst(x,y) |= PF_NO_HOLE;
                        return true; // anything next to known-not-a-hole is also not a hole
                    }

            return false;
        }
    };


    NoHoleAnnotator(Flags2d& tofill) : tofill(tofill) {}

    template<typename A>
    PixelFlag operator()(const A& a, size_t x, size_t y)
    {
        size_t filled = floodfill(tofill, a, x, y, Filler());
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
    //assert(orig != last && std::abs(int(orig.x) - int(last.x)) <= 1 && std::abs(int(orig.y) - int(last.y)) <= 1);
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
            // Update: there are no holes anymore.
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

                        // TODO: check if new polygon is fully contained within any other polygon
                        // and reject it if so
                    }
                }
        }
    return polys;
}

struct IsOutsideOfPolygonArea
{
    const Flags2d& flags;
    IsOutsideOfPolygonArea(const Flags2d& flags) : flags(flags) {}

    inline bool operator()(size_t x, size_t y) const
    {
        return !(flags(x,y) & PF_POLYGONBAND);
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

template<typename F>
static void dilate(Flags2d& solid, size_t iterations, PixelFlag oob, F& f)
{
    Flags2d tmp; // can't dilate into the source buffer because dilation checks for the same flag it sets...
    for(unsigned i = 0; i < iterations; ++i)
    {
        tmp.swap(solid); // ... so it's buffer ping-pong then.
        generate2(solid, GetValue<PixelFlag>(tmp, oob), f);
    }
}

static Image2d embossImage(const Image2d& in, const std::vector<Polygon>& polys)
{
    Image2d out = in;
    bool flop = false;
    for(size_t i = 0; i < polys.size(); ++i)
    {
        const Polygon& poly = polys[i];
        const int np = (int)poly.points.size();
        for(int k = 0; k < np; ++k)
        {
            Line2d line { poly.getPoint(k), poly.getPoint(k+1) };
            line.intersect(EmbossLine(out, flop ? WhitePixel : RedPixel));
            flop = !flop;
        }
    }
    return out;
}

static float shrinkageRatio(const Array2dAny& a, const Polygon& poly)
{
    size_t parea = poly.calcArea();
    size_t aarea = a.width() * a.height();

    return float(parea) / float(aarea);
}

static size_t calcScore(const Array2dAny& a, const Polygon& poly)
{
    size_t area = poly.calcArea(); // smaller is better

    size_t tris = poly.points.size(); // FIXME: actually use #tris

    // TODO: include relative size too
    return tris * 150 + area;
}


struct Params
{
    size_t dilation;
    size_t extraband;
    size_t segmentdist;
};

static size_t doOneImage(std::vector<Polygon>& polyout, const Image2d& img, const Params& params)
{
    //Image2d out;

    Flags2d solid;
    generate(solid, img, isNotFullyTransparent);
    dilate(solid, params.dilation, PF_EMPTY, addDilatedFlag);

    // begin closing holes. anything that is non-solid and touches the border is not a hole.
    generate2(solid, GetValue<PixelFlag>(solid, PF_NO_HOLE), NoHoleAnnotator(solid));

    // any non-solid region that isn't known to be non-hole is actually a hole. Set PF_DILATED to close it.
    generate2(solid, GetValue<PixelFlag>(solid, PF_SOLID), closeHoles);

    generate2(solid, GetValue<PixelFlag>(solid, PF_EMPTY), addBoundaryFlag);
    dilate(solid, params.extraband, PF_EMPTY, addPolygonFlag);

    /*generate(out, solid, ShowBit(PF_BOUNDARY));
    out.writePNG("boundary.png");
    generate(out, solid, ShowBit(PF_DILATED));
    out.writePNG("dilated.png");
    generate(out, solid, ShowBit(PF_POLYGONBAND));
    out.writePNG("polygonband.png");*/

    Meta2d meta;
    distributeConnectedRegions(meta, solid);

    /*generate(out, meta, ShowBitAndCC(PF_BOUNDARY));
    out.writePNG("cc.png");*/

    std::vector<Polygon> polys = generatePolygons(meta);
    std::vector<Polygon> simplepolys;
    simplepolys.reserve(polys.size());
    size_t score = 0;
    for(size_t i = 0; i < polys.size(); ++i)
    {
        Polygon tmp = polys[i].simplify(IsOutsideOfPolygonArea(solid), params.segmentdist);
        //tmp = tmp.simplifyDP(IsOutsideOfPolygonArea(solid), 3);

        simplepolys.push_back(tmp);
        const Polygon& last = simplepolys.back();
        printf("Simplified polygon[%u] from %u to %u points\n", unsigned(i), (unsigned)polys[i].points.size(), (unsigned)last.points.size());

        const size_t myscore = calcScore(img, last);
        printf("Simple poly uses %.2f%% of original pixels, score = %u\n",
            100 * shrinkageRatio(img, last), unsigned(myscore));
        score += myscore;
    }

    polyout = simplepolys;

    return score;
}

static const Params s_params[] =
{
    { 1, 3, 0 },
    { 2, 5, 0 },
    { 3, 4, 0 },
    { 4, 6, 0 },
    { 5, 6, 0 },
    { 6, 6, 0 },
    { 8, 8, 0 },
    { 10, 10, 0 }
};

struct PolyResult
{
    std::vector<Polygon> polys;
    size_t score;
};

static bool doOneImage(const char *fn)
{
    Image2d img;
    if(!img.load(fn))
    {
        printf("Failed to load image: %s\n", fn);
        return false;
    }

    PolyResult polys[Countof(s_params)];
    size_t bestscore = size_t(-1);
    size_t bestidx = 0;

    for(size_t i = 0; i < Countof(s_params); ++i)
    {
        size_t score = doOneImage(polys[i].polys, img, s_params[i]);
        printf("=> Param set #%u (dilate=%u, band=%u, linelen=%u) score: %u\n",
            unsigned(i),
            unsigned(s_params[i].dilation),
            unsigned(s_params[i].extraband),
            unsigned(s_params[i].segmentdist),
            unsigned(score)
        );
        polys[i].score = score;

        if(score < bestscore)
        {
            bestscore = score;
            bestidx = i;
        }
    }

    const PolyResult& best = polys[bestidx];

    //std::vector<unsigned> strip;
    //size_t striplen = genIndexBuffer_Strip(strip, &best.polys[0], best.polys.size());


    Image2d out = embossImage(img, best.polys);
    out.writePNG("embossed-best.png");

    return true;
}



int main(int argc, char *argv[])
{
    doOneImage("gear.png");
    //doOneImage("face.png");
    //doOneImage("menu-frame2.png");
    //doOneImage("pyramid-dragon-bg.png");
    //doOneImage("tentacles.png");
    //doOneImage("rock0002.png");
    //doOneImage("sunstatue-0001.png");
    //doOneImage("city-stairs.png");
    //doOneImage("head.png");
    //doOneImage("bg-rock-0002.png");
    //doOneImage("growth-0004.png");


    printf("Exiting.\n");
    return 0;
}
