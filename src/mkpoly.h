#pragma once

#include <vector>
#include "polygon.h"

class Image2d;

std::vector<Polygon> mkpoly_twoband(const Image2d& img);
