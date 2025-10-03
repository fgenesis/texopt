#include "atlas.h"
#include "vertexbuf.h"
#include "mkpoly.h"
#include "dt2d.h"
#include "trifill.h"
#include "stb_image_write.h"


Atlas::Atlas()
    : updateDistanceMapInterval(0)
{
}

bool Atlas::addFile(const char* fn)
{
    printf("Loading image '%s'\n", fn);

    Image2d img;
    if(!img.load(fn))
    {
        printf("Failed to load image: %s\n", fn);
        return false;
    }

    // Generate polygons enclosing image used areas
    std::vector<Polygon> polys = mkpoly_twoband(img);
    if(polys.empty())
    {
        printf("Failed to generate polygons for image: %s\n", fn);
        return false;
    }

    for(size_t i = 0; i < polys.size(); ++i)
    {
        AtlasFragment frag;
        frag.placed = false;
        frag.filename = fn;
        frag.poly = polys[i];

        const size_t striplen = genIndexBuffer_Strip(frag.strip, &frag.poly, 1, true);
        assert(striplen >= 3);

        AABB box = frag.poly.getBoundingRect();
        const size_t w = box.width(), h = box.height();
        frag.img.init(w, h);
        frag.img.copy2d(0, 0, img, box.x1, box.y1, w, h);

        polygonPointsToVertexList(frag.points, ivec2(-box.x1, -box.y1), &frag.poly, 1);

        Process(frag);

        frags.push_back(frag);
    }
    return true;
}


static void computeDT(Array2d<float> &dist, const Array2d<unsigned char>& solid)
{
    dt2d_solidToDT(dist.data(), solid.data(), dist.width(), dist.height());
}

static bool fragmentHighestUsageCmp(const AtlasFragment& a, const AtlasFragment& b)
{
    //const u32 areaA = a.sz.x * a.sz.y;
    //const u32 areaB = b.sz.x * b.sz.y;
    //return areaA > areaB; // FIXME: measurement for "unwieldy" tiles (super long, super high)
    return a.usedBlocks > b.usedBlocks;
}


void Atlas::Process(AtlasFragment& frag)
{


    indexListToTris(frag.tris, &frag.strip[0], frag.strip.size(), TRIMODE_STRIP);

    {
        const size_t w4 = (frag.img.width() + 3) / 4u;
        const size_t h4 = (frag.img.height() + 3) / 4u;
        frag.usage4x4.init(w4, h4);
        frag.distance4x4.init(w4, h4);

        Array2d<unsigned char> usageTmp;
        usageTmp.init(frag.usage4x4.width() * 4, frag.usage4x4.height() * 4);
        usageTmp.fill(0);

        trifill(usageTmp, &frag.points[0], &frag.tris[0], frag.tris.size());
        size_t used = downsample4x4(frag.usage4x4, usageTmp);
        frag.usedBlocks = used;
        size_t total = frag.usage4x4.width() * frag.usage4x4.height();
        printf("%u/%u (%f %%) of 4x4 blocks are used\n",
            (unsigned)used, (unsigned)total, 100 * (used / float(total)));

        /*
        stbi_write_png("usage4x4.png", frag.usage4x4.width(), frag.usage4x4.height(), 1, frag.usage4x4.data(), 0);
        const size_t n = usageTmp.width() * usageTmp.height();
        unsigned char *p = usageTmp.data();
        for(size_t i = 0; i < n; ++i)
            p[i] = p[i] ? 0xff : 0;
        stbi_write_png("usagefull.png", usageTmp.width(), usageTmp.height(), 1, usageTmp.data(), 0);
        */
    }

    computeDT(frag.distance4x4, frag.usage4x4);

}

bool Atlas::build()
{
    std::sort(frags.begin(), frags.end(), fragmentHighestUsageCmp);


    {
        size_t maxw = 0, maxh = 0;
        for(size_t i = 0; i < frags.size(); ++i)
        {
            maxw = std::max(maxw, frags[i].img.width());
            maxh = std::max(maxh, frags[i].img.height());
        }
        if(pixels.width() < maxw || pixels.height() < maxh)
            resize(nextPowerOf2(maxw), nextPowerOf2(maxh));
    }


    size_t fitted = 0, failed = 0;
    size_t remainBeforeRecalc = 0;

    for(size_t i = 0; i < frags.size(); ++i)
    {
        AtlasFragment& frag = frags[i];

        while(true)
        {
            //dumpState(i);

            if(_fitOne(frag, fitted == 0))
            {
                ++fitted;
                printf("Done. Fitted: %u/%u\n", (unsigned)fitted, (unsigned)frags.size());
                break;
            }



            if(!_enlarge())
            {
                ++failed;
                printf("Failed to fit tile: %s\n", frag.filename.c_str());
                break;
            }
        }

        if(remainBeforeRecalc)
            --remainBeforeRecalc;
        else
        {
            remainBeforeRecalc = this->updateDistanceMapInterval;
            updateDT();
        }
    }

    printf("Atlas: %u fitted, %u failed\n", (unsigned)fitted, (unsigned)failed);

    return !failed;
}

bool Atlas::_fitOne(AtlasFragment& frag, bool first)
{
    printf("Fitting %s\n", frag.filename.c_str());

    const size_t w4 = frag.usage4x4.width();
    const size_t h4 = frag.usage4x4.height();
    const size_t imaxw4 = usage4x4.width() - w4 + 1;
    const size_t imaxh4 = usage4x4.height() - h4 + 1;

    ivec2 bestpos(0, 0);

    if(!first) // First tile goes in the upper left corner, period (otherwise trying to compute the distance transform will end up unhappy)
    {
        bool found = false;
        float bestscore = std::numeric_limits<float>::infinity();
        for(size_t iy = 0; iy < imaxh4; ++iy)
            for(size_t ix = 0; ix < imaxw4; ++ix)
            {
                float score;
                if(tryFitAt_Coarse(&score, frag, ix, iy, bestscore) && score < bestscore)
                {
                    found = true;
                    bestscore = score;
                    bestpos = ivec2(ix, iy);
                    if(!score) // perfect fit?
                        goto finish;
                }
            }
        if(!found)
            return false; // no spot found
    }

finish:

    printf("Fit %s at (%d, %d)\n", frag.filename.c_str(), bestpos.x, bestpos.y);
    frag.location = bestpos * 4;
    frag.placed = true;

    // TODO: finetune-nudge?

    // add in
    tridraw(pixels, bestpos * 4, &frag.points[0], &frag.tris[0], frag.tris.size(), frag.img);

    // update usage map so that now occupied blocks are marked as such
    for(size_t y = 0; y < h4; ++y)
        for(size_t x = 0; x < w4; ++x)
            usage4x4(bestpos.x + x, bestpos.y + y) += frag.usage4x4(x, y);

    return true;
}

void Atlas::resize(size_t w, size_t h)
{
    printf("Resize atlas to (%u x %u)\n", (unsigned)w, (unsigned)h);
    pixels.resize(w, h);

    const size_t w4 = (w + 3) / 4u;
    const size_t h4 = (h + 3) / 4u;
    usage4x4.resize(w4, h4);
    distance4x4.init(w4, h4);
    updateDT();
}

bool Atlas::tryFitAt_Coarse(float *pscore, const AtlasFragment& frag, size_t xo, size_t yo, float curscore) const
{
    const size_t fw = frag.usage4x4.width();
    const size_t fh = frag.usage4x4.height();
    const size_t aw = usage4x4.width();
    const size_t ah = usage4x4.height();
    if(xo + fw > aw || yo + fh > ah)
        return false;

    float score = 0;
    for(size_t y = 0; y < fh; ++y)
    {
        const unsigned char * const pa =      usage4x4   .row(y + yo) + xo;
        const float *         const sa =      distance4x4.row(y + yo) + xo;
        const unsigned char * const pf = frag.   usage4x4.row(y);
        const float *         const sf = frag.distance4x4.row(y);

        for(size_t x = 0; x < fw; ++x)
            if(pa[x] && pf[x]) // Both set? Collision, done here
                return false;
            else //if(pa[x] || pf[x])
            {
                // Penalize unfilled blocks
                //if(!pa[x] && !pf[x])
                    score += sa[x];
                //if(!pf[x])
                //    score += sf[x];
            }

        // Only check this occasionally, no need to do this on every block
        if(score > curscore) // Won't fit any better
            return false;
    }

    *pscore = score;
    return true;
}

bool Atlas::_enlarge()
{
    if(pixels.width() < pixels.height())
        resize(pixels.width() * 2, pixels.height());
    else
        resize(pixels.width(), pixels.height() * 2);

    return true;
}

void Atlas::updateDT()
{
    computeDT(distance4x4, usage4x4);
}

void Atlas::renderCurrentState(Image2d& out)
{
    out.init(pixels.width(), pixels.height());

    const size_t w4 = usage4x4.width();
    const size_t h4 = usage4x4.height();

    Pixel pix;
    pix.a = 0xff;

    // distance grid
    for(size_t by = 0; by < h4; ++by)
        for(size_t bx = 0; bx < w4; ++bx)
        {
            float d = distance4x4(bx, by) * 256 * 2;
            pix.r = pix.g = pix.b = std::min((int)d, 0xff);
            for(size_t y = 0; y < 4; ++y)
                for(size_t x = 0; x < 4; ++x)
                    out(bx*4+x, by*4+y) = pix;


        }

    // pixel data
    for(size_t i = 0; i < frags.size(); ++i)
    {
        AtlasFragment& frag = frags[i];
        if(!frag.placed)
            continue;

        tridraw(out, frag.location, &frag.points[0], &frag.tris[0], frag.tris.size(), frag.img);
    }


    // overlay
    pix.r = 0;
    pix.g = 0xff;
    pix.b = 0;
    for(size_t by = 0; by < h4; ++by)
        for(size_t bx = 0; bx < w4; ++bx)
            if(usage4x4(bx, by))
            {
                out(bx*4, by*4) = pix;
                out(bx*4+1, by*4) = pix;
                out(bx*4, by*4+1) = pix;
            }


    pix.r = 0xff;
    pix.g = 0;
    pix.b = 0;
    for(size_t i = 0; i < frags.size(); ++i)
    {
        AtlasFragment& frag = frags[i];
        if(!frag.placed)
            continue;

        triwireframe(out, frag.location, &frag.points[0], &frag.tris[0], frag.tris.size(), pix);
    }
}


void Atlas::dumpState(size_t i)
{
    Image2d dbg;
    renderCurrentState(dbg);
    char buf[128];
    //sprintf(buf, "_atlas/status%05u__%ux%u.png", (unsigned)i, (unsigned)pixels.width(), (unsigned)pixels.height());
    sprintf(buf, "_atlas/status%05u.png", (unsigned)i);
    dbg.writePNG(buf);
}

size_t Atlas::exportVerticesU(std::vector<uvec2>& dst)
{
    const size_t oldsize = dst.size();
    for(size_t i = 0; i < frags.size(); ++i)
    {
        AtlasFragment& frag = frags[i];
        if(!frag.placed)
            continue;

        dst.insert(dst.end(), frag.points.begin(), frag.points.end());
    }
    return dst.size() - oldsize;
}

size_t Atlas::exportVerticesF(std::vector<vec2>& dst)
{
    std::vector<uvec2> tmp;
    const size_t N = exportVerticesU(tmp);
    dst.reserve(dst.size() + N);

    // Double precision because this is an offline preprocessing step where as little precision loss as possible is good
    const double dw = double(pixels.width());
    const double dh = double(pixels.height());
    const dvec2 halfPixel(0.5 / dw, 0.5 / dh);
    const dvec2 dsize(dw, dh);
    for(size_t i = 0; i < N; ++i)
    {
        vec2 dv = vec2((dvec2(tmp[i]) / dsize) + halfPixel);
        dst.push_back(dv);
    }
    return N;
}

size_t Atlas::exportIndices(std::vector<unsigned>& dst, bool keepRestart)
{
    const size_t oldsize = dst.size();
    const bool degenerate = !keepRestart;
    size_t offset = 0;

    for(size_t i = 0; i < frags.size(); ++i)
    {
        AtlasFragment& frag = frags[i];
        if(!frag.placed)
            continue;

        const size_t N = frag.strip.size();
        unsigned prev = 0;
        for(size_t k = 0; k < N; ++k)
        {
            unsigned idx = frag.strip[k];
            if(idx == RESTART)
            {
                if(degenerate)
                {
                    dst.push_back(prev + offset);
                    dst.push_back(frag.strip[k+1] + offset);
                }
                else
                {
                    dst.push_back(idx);
                }
            }
            else
            {
                dst.push_back(idx + offset);
                prev = idx;
            }
        }

        offset += frag.points.size();
    }
    return dst.size() - oldsize;
}
