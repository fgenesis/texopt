#pragma once

#include "polygon.h"
#include "vec.h"

enum { RESTART = 0xffffffff };

enum TriMode
{
	TRIMODE_STRIP,
	TRIMODE_FAN,
	//TRIMODE_TRIS,
};

size_t genIndexBuffer_Strip(std::vector<unsigned>& dst, const Polygon *polys, size_t n, bool useRestart);

void polygonPointsToVertexList(std::vector<uvec2>& points, ivec2 offset, const Polygon *polys, size_t n);
void indexListToTris(std::vector<Tri>& tris, const unsigned* indices, size_t n, TriMode trimode);

// returns 0 when failed
size_t strip2fan(std::vector<unsigned>& dst, const unsigned *stripIndexes, size_t n);

