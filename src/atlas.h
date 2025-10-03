#pragma once

#include "polygon.h"
#include "image2d.h"

struct AtlasFragment
{
    Image2d img;
    Polygon poly;

    std::vector<unsigned> strip;
    std::vector<uvec2> points;
    std::vector<Tri> tris;


    Array2d<unsigned char> usage4x4;
    Array2d<float> distance4x4;
    size_t usedBlocks;
    std::string filename;
    ivec2 location;
    bool placed;
};

class Atlas
{
public:
    Atlas();
    bool addFile(const char *fn);
    static void Process(AtlasFragment &frag);

    bool build();
    void resize(size_t w, size_t h);
    bool tryFitAt_Coarse(float *pscore, const AtlasFragment& frag, size_t xo, size_t yo, float curscore) const;
    void renderCurrentState(Image2d& out);
    void dumpState(size_t i);
    size_t exportVerticesU(std::vector<uvec2> &dst);
    size_t exportVerticesF(std::vector<vec2> &dst);
    size_t exportIndices(std::vector<unsigned> &dst, bool keepRestart);

    size_t updateDistanceMapInterval;

private:
    bool _enlarge();
    bool _fitOne(AtlasFragment& frag, bool first);
    std::vector<AtlasFragment> frags;

    Image2d pixels;
    Array2d<unsigned char> usage4x4;
    Array2d<float> distance4x4;
    void updateDT();
};
