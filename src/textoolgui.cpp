#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error Require SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include <stdio.h>

#include "stb_image.h"
#include "util.h"
#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"
#include <assert.h>
#include "image2d.h"


SDL_Window* window;
SDL_Renderer* renderer;
static ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

static const Pixel transparent = { 0, 0, 0, 0 };

// input tex
static Image2d imgin;
static AABB inAABB;
static SDL_Texture *tex;
static glm::ivec2 texsize;
static glm::ivec2 textranslate;
static glm::vec2 textransperc;
static glm::ivec2 clickpos;
static float scale = 1.0f;

// output tex
static Image2d imgout;

// display options
static bool showbbox = true;


static void onUpdateTex()
{
    inAABB = imgin.getAlphaRegion();

    if(tex)
    {
        SDL_DestroyTexture(tex);
        tex = NULL;
    }

    tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, (int)imgin.width(), (int)imgin.height());
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_UpdateTexture(tex, NULL, imgin.data(), 4 * imgin.width());
    texsize = glm::vec2((float)imgin.width(), (float)imgin.height());
}

static void setTex(const char *fn)
{
    if(!imgin.load(fn))
        return;
    onUpdateTex();
}

static void resetRotPoint()
{
    textranslate = glm::ivec2(0);
    textransperc = glm::vec2(0);
}


static void fitToSize()
{
    const glm::ivec2 rotPointTexAbs = (texsize / 2) + textranslate;
    const glm::ivec2 rotPointAABBAbs = rotPointTexAbs - glm::ivec2(inAABB.x1, inAABB.y1);

    const unsigned maxX = glm::max(rotPointAABBAbs.x, (int)inAABB.width() - rotPointAABBAbs.x);
    const unsigned maxY = glm::max(rotPointAABBAbs.y, (int)inAABB.height() - rotPointAABBAbs.y);

    const unsigned texSizeX = nextPowerOf2(2 * maxX);
    const unsigned texSizeY = nextPowerOf2(2 * maxY);

    const glm::ivec2 newTexCenter(texSizeX / 2, texSizeY / 2);
    const glm::ivec2 newTexTopLeft = newTexCenter - rotPointAABBAbs;

    imgout.init(texSizeX, texSizeY);
    imgout.fill(transparent);
    imgout.copy2d(newTexTopLeft.x, newTexTopLeft.y, imgin, inAABB.x1, inAABB.y1, inAABB.width(), inAABB.height());

    imgin = imgout;
    onUpdateTex();
    resetRotPoint();

}

static void drawWindow()
{
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    ImGui::Text("RotPRel: %d, %d", textranslate.x, textranslate.y);

    const glm::ivec2 rotPoint = (texsize / 2)+ textranslate;
    ImGui::Text("RotPAbs: %d, %d", rotPoint.x, rotPoint.y);

    ImGui::Checkbox("BBox", &showbbox);

    if(ImGui::Button("Center rot point"))
        resetRotPoint();

    if(ImGui::Button("Fit to size/rot"))
        fitToSize();
}

static void drawMain()
{
    if(tex)
    {
        int w, h;
        int err = SDL_GetRendererOutputSize(renderer, &w, &h);
        assert(!err);
        const float fw = float(w);
        const float fh =  float(h);
        const float fw2 = 0.5f * float(w);
        const float fh2 = 0.5f * float(h);


        SDL_RenderCopy(renderer, tex, NULL, NULL);
        unsigned ticks = SDL_GetTicks();
        const float timer = float(ticks) * 0.1f;

        const glm::vec2 ts = glm::vec2(texsize);
        const glm::vec2 ts2 = 0.5f * ts;
        const float texaspect = ts.x / ts.y;
        const float texmin = glm::min(ts.x, ts.y);

        const float tx = texmin * texaspect;
        const float ty = texmin;

        glm::mat4 om = glm::translate(glm::vec3(glm::vec2(clickpos), 0.0f)); // offset matrix: place object
        glm::mat4 tm = glm::translate(glm::vec3(-ts2 * textransperc, 0.0f)); // translation relative to rotation point
        glm::mat4 rm = glm::rotate(timer, glm::vec3(0.0f, 0.0f, 1.0f));      // rotation
        glm::mat4 sm = glm::scale(glm::vec3(scale));                         // scale
        glm::mat4 m = om * rm * sm * tm;

        glm::vec4 ul = m * glm::vec4( ts2.x,  ts2.y,  0.0f, 1.0f);
        glm::vec4 ur = m * glm::vec4(-ts2.x,  ts2.y,  0.0f, 1.0f);
        glm::vec4 lr = m * glm::vec4(-ts2.x, -ts2.y,  0.0f, 1.0f);
        glm::vec4 ll = m * glm::vec4( ts2.x, -ts2.y,  0.0f, 1.0f);


        static const SDL_FPoint tc_ul = { 1, 1 };
        static const SDL_FPoint tc_ur = { 0, 1 };
        static const SDL_FPoint tc_lr = { 0, 0 };
        static const SDL_FPoint tc_ll = { 1, 0 };
        static const SDL_Color col = { 255, 255, 255, 170 };
        SDL_Vertex verts[] =
        {
            { {ul.x + fw2, ul.y + fh2}, col, tc_ul },
            { {ur.x + fw2, ur.y + fh2}, col, tc_ur },
            { {lr.x + fw2, lr.y + fh2}, col, tc_lr },
            { {ll.x + fw2, ll.y + fh2}, col, tc_ll }
        };
        int indices[] =
        {
            0, 1, 2,
            2, 3, 0
        };
        SDL_RenderGeometry(renderer, tex, verts, Countof(verts), indices, Countof(indices));


        if(showbbox)
        {
            // draw image bounding box
            SDL_FPoint points[] =
            {
                {ul.x + fw2, ul.y + fh2},
                {ur.x + fw2, ur.y + fh2},
                {lr.x + fw2, lr.y + fh2},
                {ll.x + fw2, ll.y + fh2},
                {ul.x + fw2, ul.y + fh2}
            };
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
            SDL_RenderDrawLinesF(renderer, points, Countof(points));

            // draw alpha bounding box
            glm::vec4 ula = m * glm::vec4(inAABB.x1 - ts2.x, inAABB.y1 - ts2.y,  0.0f, 1.0f);
            glm::vec4 ura = m * glm::vec4(inAABB.x2 - ts2.x, inAABB.y1 - ts2.y,  0.0f, 1.0f);
            glm::vec4 lra = m * glm::vec4(inAABB.x2 - ts2.x, inAABB.y2 - ts2.y,  0.0f, 1.0f);
            glm::vec4 lla = m * glm::vec4(inAABB.x1 - ts2.x, inAABB.y2 - ts2.y,  0.0f, 1.0f);
            SDL_FPoint pointsa[] =
            {
                {ula.x + fw2, ula.y + fh2},
                {ura.x + fw2, ura.y + fh2},
                {lra.x + fw2, lra.y + fh2},
                {lla.x + fw2, lla.y + fh2},
                {ula.x + fw2, ula.y + fh2}
            };
            SDL_SetRenderDrawColor(renderer, 255, 100, 255, 150);
            SDL_RenderDrawLinesF(renderer, pointsa, Countof(pointsa));
        }

    }
}

static void onFocusPixel_R(int x, int y)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glm::vec2 perc = glm::vec2(x, y) / glm::vec2(w, h);
    textransperc = (perc - glm::vec2(0.5f)) * 2.0f;
    //printf("perc = %f, %f\n", perc.x, perc.y);
    glm::vec2 ts = glm::vec2(texsize);
    glm::vec2 scaled = ts * perc;
    //printf("pixel = %d, %d\n", int(scaled.x), int(scaled.y));
    textranslate = glm::ivec2(scaled) - (texsize / 2);
    //printf("move = %d, %d\n", textranslate.x, textranslate.y);
}

static void onFocusPixel_L(int x, int y)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    clickpos = glm::ivec2(x, y) - (glm::ivec2(w, h) / 2);
}

static void onFocusPixel(int x, int y, unsigned buttonmask)
{
    if(buttonmask & 1)
        onFocusPixel_L(x, y);
    else if(buttonmask & 4)
        onFocusPixel_R(x, y);
}



// Main code
int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("textool GUI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);

    // Setup SDL_Renderer instance
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer_Init(renderer);

    setTex("test.png");

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(window))
            {
                if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                    done = true;
            }

            if(event.type == SDL_DROPFILE)
            {
                printf("Drop file: %s\n", event.drop.file);
                setTex(event.drop.file);
            }

            if(!io.WantCaptureMouse)
            {
                if(event.type == SDL_MOUSEBUTTONDOWN)
                    onFocusPixel(event.button.x, event.button.y, (1 << (event.button.button-1)));
                else if(event.type == SDL_MOUSEMOTION && event.motion.state)
                    onFocusPixel(event.motion.x, event.motion.y, event.motion.state);

                if(event.type == SDL_MOUSEWHEEL)
                {
                    float m = 1.0f + (event.wheel.y * 0.15f);
                    scale *= m;
                }
            }
        }

        ImGui_ImplSDLRenderer_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Controls");
        drawWindow();
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);
        drawMain();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
