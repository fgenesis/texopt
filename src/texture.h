#pragma once

#include "image2d.h"
#include <glm/glm.hpp>

struct SDL_Texture;

struct Texture
{
    Texture();

    void import(Image2d& img);
    void rebuildAABB(Image2d& img);

    AABB aabb;
    SDL_Texture *tex;
    glm::vec2 ts;
    glm::uvec2 size;
    glm::ivec2 centerOffs; // rotation point
};
