#pragma once

#include "array2d.h"
#include "vec.h"
#include "polygon.h"

class Image2d;

size_t trifill(Array2d<unsigned char>& out, const uvec2 *points, const Tri *tris, size_t ntris);
void tridraw(Image2d& out, ivec2 offset, const uvec2 *points, const Tri *tris, size_t ntris, const Image2d& src);
size_t downsample4x4(Array2d<unsigned char>& out, Array2d<unsigned char>& in);

void triwireframe(Image2d& out, ivec2 offset, const uvec2 *points, const Tri *tris, size_t ntris, Pixel color);
