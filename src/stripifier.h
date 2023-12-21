#pragma once

#include <vector>

size_t stripify(std::vector<unsigned>& out, const unsigned *indices, size_t indexcount, size_t vertexcount, unsigned restartindex);
