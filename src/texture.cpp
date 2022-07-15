#include "texture.h"

#include <SDL.h>
extern SDL_Renderer *renderer;

Texture::Texture()
    : tex(NULL)
{
}

void Texture::import(Image2d& img)
{
    rebuildAABB(img);
    if(tex)
    {
        SDL_DestroyTexture(tex);
        tex = NULL;
    }

    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, (int)img.width(), (int)img.height());
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(tex, NULL, img.data(), 4 * img.width());
    size = glm::uvec2(img.width(), img.height());

    ts = glm::vec2(size);
}

void Texture::rebuildAABB(Image2d& img)
{
    aabb = img.getAlphaRegion();

}
