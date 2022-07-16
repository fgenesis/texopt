#include <SDL.h>

#if !SDL_VERSION_ATLEAST(2,0,17)
#error Require SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string>
#include <direct.h>
#include <map>
#include <fstream>
#include <sstream>

#include "stb_image.h"
#include "util.h"
#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"
#include <assert.h>
#include "image2d.h"
#include <nfd.h>
#include "texture.h"

SDL_Window* window;
SDL_Renderer* renderer;
static ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);

static const Pixel transparent = { 0, 0, 0, 0 };

// input tex
static Texture texin;
static Image2d imgin;
static glm::ivec2 clickpos;
static float rescale = 1.0f;
static float displayscale = 1.0f;
static std::string texFn;

// output tex
static Texture texout;
static Image2d imgout;
char s_savename[64];
static bool s_savenameChanged = false;

static std::string s_outDir;

// display options
static bool showbbox = true;
static bool autoloadsettings = true;


static std::string saveCurrentSettings();
static std::string loadCurrentSettings();
static void fitToSize(const Image2d& img, const Texture& tex);


struct PerFileSettings
{
    PerFileSettings()
        : scale(1.0f), offx(0), offy(0) {}
    std::string outfn;
    float scale;
    int offx, offy;
};

typedef std::map<std::string, PerFileSettings> SettingsMap;
static SettingsMap settings;

static void loadSettings(const std::string& dir)
{
    settings.clear();
    std::string fn = dir + "textool.ini";
    std::ifstream f(fn.c_str(), std::ifstream::in);
    if(!f)
        return;

    std::string line, tfile;

    while(std::getline(f, line))
    {
        if(!line.length())
            continue;

        if(line[0] == '=')
        {
            s_outDir = line.substr(1);
        }
        else
        {
            std::istringstream is(line);
            PerFileSettings se;
            if(is >> tfile >> se.outfn >> se.offx >> se.offy >> se.scale)
                settings[tfile] = se;
        }
    }
    f.close();
    printf("Loaded settings for %u files in %s\n", (unsigned)settings.size(), dir.c_str());
}

static void saveSettings(const std::string& dir)
{
    std::string fn = dir + "textool.ini";
    std::ofstream f(fn.c_str(), std::fstream::out);

    if(s_outDir.length())
        f << '=' << s_outDir << "\n";

    for(SettingsMap::const_iterator it = settings.begin(); it != settings.end(); ++it)
    {
        if(it->first.find_first_of(" \t") != std::string::npos)
        {
            printf("Not saving '%s' because it contains spaces\n", it->first.c_str());
            continue;
        }
        const PerFileSettings& se = it->second;
        f << it->first << "\t" << se.outfn << "\t" << se.offx << "\t" << se.offy << "\t" << se.scale << "\n";
    }
    f.close();
}

static glm::uvec2 nextPowerOf2(glm::uvec2 in)
{
    return glm::uvec2(nextPowerOf2(in.x), nextPowerOf2(in.y));
}


static void onImgInChanged(const char *fn)
{
    texout.import(imgout);

    // -- apply stored settings --
    if(fn)
    {
        std::string olddn = saveCurrentSettings();
        std::string dn = dirname(fn);

        texFn = fn;
        loadCurrentSettings();
    }
}

static void onImgOutChanged()
{
    texout.import(imgout);
    texout.centerOffs = glm::ivec2(0);
}

static void applySettings(const PerFileSettings& se)
{
    texin.centerOffs = glm::ivec2(se.offx, se.offy);
    rescale = se.scale;
    strncpy(s_savename, se.outfn.c_str(), sizeof(s_savename));
    s_savenameChanged = true;
    fitToSize(imgin, texin);
}

static void applySettings(const std::string& name)
{
    const std::string fn = filename(name);
    SettingsMap::const_iterator it = settings.find(fn);
    if(it != settings.end())
        applySettings(it->second);
    else
    {
        s_savenameChanged = true;
        s_savename[0] = 0;
    }
}

static bool updateSettings()
{
    if(texFn.length() && s_savename[0])
    {
        std::string fn = filename(texFn);
        PerFileSettings& se = settings[fn];
        se.offx = texin.centerOffs.x;
        se.offy = texin.centerOffs.y;
        se.scale = rescale;
        se.outfn = s_savename;
        return true;
    }
    return false;
}

static std::string saveCurrentSettings()
{
    std::string olddn = dirname(texFn);
    saveSettings(olddn);
    return olddn;
}

static std::string loadCurrentSettings()
{
    std::string olddn = dirname(texFn);
    loadSettings(olddn);

    if(autoloadsettings)
        applySettings(texFn);

    return olddn;
}


static bool setTex(const char *fn)
{
    if(!imgin.load(fn))
        return false;
    imgout = imgin;
    texin.import(imgin);
    onImgInChanged(fn);
    return true;
}

static void makeCutout(Image2d& dst, const Image2d& src, const AABB& aabb)
{
    dst.init(aabb.width(), aabb.height());
    dst.copy2d(0, 0, src, aabb.x1, aabb.y1, aabb.width(), aabb.height());
}

static glm::ivec2 newTexCenter;
static glm::ivec2 newTexTopLeft;
static unsigned newTexSizeX, newTexSizeY;

static void _updateFit(const Image2d& src, const AABB& aabb, const glm::ivec2& translate)
{
    const glm::ivec2 ts = glm::ivec2(src.width(), src.height());
    if(!(ts.x && ts.y))
        return;

    const glm::ivec2 rotPointTexAbs = (ts / 2) + translate;
    const glm::ivec2 rotPointAABBAbs = rotPointTexAbs - glm::ivec2(aabb.x1, aabb.y1);

    const unsigned maxX = glm::max(rotPointAABBAbs.x, (int)aabb.width() - rotPointAABBAbs.x);
    const unsigned maxY = glm::max(rotPointAABBAbs.y, (int)aabb.height() - rotPointAABBAbs.y);

    newTexSizeX = nextPowerOf2(2 * maxX);
    newTexSizeY = nextPowerOf2(2 * maxY);

    newTexCenter = glm::ivec2(newTexSizeX / 2, newTexSizeY / 2);
    newTexTopLeft = newTexCenter - rotPointAABBAbs;
}

static void updateFit()
{
    _updateFit(imgin, texin.aabb, texin.centerOffs);
}

static void _fitToSize(const Image2d& src, const AABB& aabb, const glm::ivec2& translate)
{
    _updateFit(src, aabb, translate);

    imgout.init(newTexSizeX, newTexSizeY);
    imgout.fill(transparent);
    imgout.copy2d(newTexTopLeft.x, newTexTopLeft.y, src, aabb.x1, aabb.y1, aabb.width(), aabb.height());

    onImgOutChanged();
}

static void fitToSize(const Image2d& img, const Texture& tex)
{
    _updateFit(img, tex.aabb, tex.centerOffs);
    _fitToSize(img, tex.aabb, tex.centerOffs);
}

static glm::uvec2 getScaledSize(const Texture& tex)
{
    return glm::max(glm::uvec2(1), glm::uvec2(tex.ts * rescale));
}

static void scaleImg()
{
    if(rescale >= 1.0f)
        return;

    glm::uvec2 sz = getScaledSize(texout);
    Image2d sc(sz.x, sz.y);
    sc.copyscaled(imgout);

    const glm::uvec2 psz = nextPowerOf2(sz);
    const glm::uvec2 halfborder = (psz - sz) / 2u;
    Image2d out;
    out.init(psz.x, psz.y);
    out.fill(transparent);
    out.copy2d(halfborder.x, halfborder.y, sc, 0, 0, sc.width(), sc.height());

    AABB aabb = out.getAlphaRegion();
    _updateFit(out, aabb, glm::ivec2(0));
    _fitToSize(out, aabb, glm::ivec2(0));
}

static void ensureLastDirsep(std::string& s)
{
    if(!s.empty() && s.back() != DIRSEP)
        s += DIRSEP;
}

static void ensureOutDir()
{
    if(s_outDir.empty())
    {
        char buf[1024];
        char *wd = getcwd(buf, sizeof(buf));
        if(wd)
        {
            s_outDir = wd;
            ensureLastDirsep(s_outDir);
        }
    }
}

static void pickOutDir()
{
    ensureOutDir();
    nfdchar_t *outPath = NULL;
    nfdresult_t res = NFD_PickFolder(s_outDir.c_str(), &outPath);
    if(res == NFD_OKAY)
    {
        s_outDir = outPath;
        free(outPath);
        ensureLastDirsep(s_outDir);
    }
}

static bool saveImg(const char *fn)
{
    if(!fn || !*fn)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", "file name plz", window);
        return false;
    }

    std::string out = s_outDir + fn + ".png";
    if(!imgout.writePNG(out.c_str()))
    {
        out = "Failed to write PNG:\n" + out;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!", out.c_str(), window);
        return false;
    }
    return true;
}

static void doBatch()
{
    size_t good = 0;
    const std::string dn = dirname(texFn);
    std::string fn;
    SettingsMap copy = settings;
    for(SettingsMap::const_iterator it = copy.begin(); it != copy.end(); ++it)
    {
        fn = dn + it->first;
        if(setTex(fn.c_str()))
        {
            const PerFileSettings& se = it->second;
            applySettings(se);
            //fitToSize(imgin, texin); // already called by applySettings()
            scaleImg();
            good += saveImg(se.outfn.c_str());
        }

    }

    std::ostringstream os;
    if(good == copy.size())
    {
        //os << "All " << good << " textures saved properly";
        //SDL_ShowSimpleMessageBox(0, "All done", os.str().c_str(), window);
    }
    else
    {
        os << good << " textures saved properly, " << (copy.size() - good) << " failed";
        SDL_ShowSimpleMessageBox(0, "Ehhh...!", os.str().c_str(), window);
    }
}


static void drawWindow()
{
    if(s_savenameChanged)
    {
        s_savenameChanged = false;
        ImGui::ClearActiveID(); // HACK
    }

    ImGui::ColorEdit3("clear color", (float*)&clear_color);

    const glm::ivec2 bb((int)texin.aabb.width(), (int)texin.aabb.height());
    ImGui::Text("In Tex size: (%d, %d); BBox: (%d, %d)", texin.size.x, texin.size.y, bb.x, bb.y);

    const glm::ivec2 bbs = glm::ivec2(glm::vec2(bb) * rescale);
    const glm::uvec2 bb2 = nextPowerOf2(glm::uvec2(glm::vec2(bb) * rescale));
    ImGui::Text("Scaled: (%i, %i); NPOT: (%u, %u) ", bbs.x, bbs.y, bb2.x, bb2.y);

    ImGui::Text("Out Tex size: (%d, %d); BBox: (%d, %d)", texout.size.x, texout.size.y, (int)texout.aabb.width(), (int)texout.aabb.height());


    //const glm::ivec2 rotPoint = glm::ivec2(texsize) / 2 + textranslate;
    //ImGui::Text("Rot Rel: (%d, %d); Abs: (%d, %d)", textranslate.x, textranslate.y,  rotPoint.x, rotPoint.y);

    ImGui::Checkbox("BBox", &showbbox);
    ImGui::SameLine();
    if(ImGui::Button("Center rot point"))
        texin.centerOffs = glm::ivec2(0);
    ImGui::SameLine();
    if(ImGui::Button("Fit to size/rot"))
        fitToSize(imgin, texin);

    ImGui::SliderFloat("##scaleslider", &rescale, 0, 1);
    ImGui::SameLine();
    if(ImGui::Button("Rescale"))
        scaleImg();

    ImGui::Text("%u settings here", (unsigned)settings.size());
    ImGui::SameLine();
    ImGui::Checkbox("cfg autoload", &autoloadsettings);
    ImGui::SameLine();
    if(ImGui::Button("Load"))
        loadCurrentSettings();
    ImGui::SameLine();
    if(ImGui::Button("Save"))
        saveCurrentSettings();

    ImGui::SameLine();
    if(updateSettings())
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Recording settings...");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Not saved; put outname");

    if(ImGui::Button("Dst dir?"))
        pickOutDir();
    ImGui::SameLine();
    ImGui::TextUnformatted(s_outDir.c_str());


    ImGui::Text("Output name:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::InputText("##savename", s_savename, sizeof(s_savename), ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    if(ImGui::Button("Write Tex"))
        saveImg(s_savename);
    ImGui::SameLine();
    if(ImGui::Button("Batch save"))
        doBatch();
}


struct Viewport
{
    float fw, fh, fw2, fh2;

    Viewport()
    {
        int w, h;
        int err = SDL_GetRendererOutputSize(renderer, &w, &h);
        assert(!err);
        fw = float(w);
        fh =  float(h);
        fw2 = 0.5f * float(w);
        fh2 = 0.5f * float(h);
    }
};

static glm::mat4 genMatrix(glm::uvec2 texsz, glm::vec2 where, glm::vec2 offset, float rotation, float scale)
{
    const glm::vec2 ts = glm::vec2(texsz);
    const glm::vec2 ts2 = 0.5f * ts;

    glm::vec2 perc = (glm::vec2(offset) + ts2) / ts;
    glm::vec2 textransperc = (perc - glm::vec2(0.5f)) * 2.0f;

    glm::mat4 om = glm::translate(glm::vec3(glm::vec2(where), 0.0f)); // offset matrix: place object
    glm::mat4 tm = glm::translate(glm::vec3(-ts2 * textransperc, 0.0f)); // translation relative to rotation point
    glm::mat4 rm = glm::rotate(rotation, glm::vec3(0.0f, 0.0f, 1.0f));      // rotation
    glm::mat4 sm = glm::scale(glm::vec3(scale));        // scale
    glm::mat4 m = om * rm * sm * tm;
    return m;
}

static void drawTexQuad(const Viewport& vp, const Texture& tex, const glm::mat4& m, glm::uvec4 bbox)
{
    const glm::vec2 ts2 = tex.ts * 0.5f;

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
        { {ul.x + vp.fw2, ul.y + vp.fh2}, col, tc_ul },
        { {ur.x + vp.fw2, ur.y + vp.fh2}, col, tc_ur },
        { {lr.x + vp.fw2, lr.y + vp.fh2}, col, tc_lr },
        { {ll.x + vp.fw2, ll.y + vp.fh2}, col, tc_ll }
    };
    int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };
    SDL_RenderGeometry(renderer, tex.tex, verts, Countof(verts), indices, Countof(indices));


    if(bbox.a)
    {
        // draw image bounding box
        {
            SDL_FPoint points[] =
            {
                {ul.x + vp.fw2, ul.y + vp.fh2},
                {ur.x + vp.fw2, ur.y + vp.fh2},
                {lr.x + vp.fw2, lr.y + vp.fh2},
                {ll.x + vp.fw2, ll.y + vp.fh2},
                {ul.x + vp.fw2, ul.y + vp.fh2}
            };
            SDL_SetRenderDrawColor(renderer, bbox.r, bbox.g, bbox.b, bbox.a);
            SDL_RenderDrawLinesF(renderer, points, Countof(points));
        }

        // draw alpha bounding box
        {
            glm::vec2 ts2 = tex.ts * 0.5f;
            glm::vec4 ula = m * glm::vec4(tex.aabb.x1 - ts2.x, tex.aabb.y1 - ts2.y,  0.0f, 1.0f);
            glm::vec4 ura = m * glm::vec4(tex.aabb.x2 - ts2.x, tex.aabb.y1 - ts2.y,  0.0f, 1.0f);
            glm::vec4 lra = m * glm::vec4(tex.aabb.x2 - ts2.x, tex.aabb.y2 - ts2.y,  0.0f, 1.0f);
            glm::vec4 lla = m * glm::vec4(tex.aabb.x1 - ts2.x, tex.aabb.y2 - ts2.y,  0.0f, 1.0f);
            SDL_FPoint points[] =
            {
                {ula.x + vp.fw2, ula.y + vp.fh2},
                {ura.x + vp.fw2, ura.y + vp.fh2},
                {lra.x + vp.fw2, lra.y + vp.fh2},
                {lla.x + vp.fw2, lla.y + vp.fh2},
                {ula.x + vp.fw2, ula.y + vp.fh2}
            };
            SDL_SetRenderDrawColor(renderer, 255, 100, 255, bbox.a);
            SDL_RenderDrawLinesF(renderer, points, Countof(points));
        }

    }

}

static void drawMain()
{
    unsigned ticks = SDL_GetTicks();
    const float timer = float(ticks) * 0.1f;

    const Viewport vp;
    unsigned boxalpha = showbbox ? 150 : 0;
    if(texin.tex)
    {
        SDL_RenderCopy(renderer, texin.tex, NULL, NULL);

        const glm::mat4 m = genMatrix(texin.size, glm::vec2(clickpos), glm::vec2(texin.centerOffs), timer, displayscale);
        drawTexQuad(vp, texin, m, glm::uvec4(255, 255, 255, boxalpha));

    }

    if(texout.tex)
    {
        const glm::mat4 m = genMatrix(texout.size, glm::vec2(0), glm::vec2(texout.centerOffs), timer, displayscale * rescale);
        drawTexQuad(vp, texout, m, glm::uvec4(128, 255, 128, boxalpha));
    }
}


static void onFocusPixel_R(int x, int y)
{
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    glm::vec2 perc = glm::vec2(x, y) / glm::vec2(w, h);
    glm::vec2 scaled = texin.ts * perc;
    texin.centerOffs = glm::ivec2(glm::ivec2(scaled) - (glm::ivec2(texin.size) / 2));
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

#ifdef _DEBUG
    setTex("test.png");
#endif

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
                    displayscale *= m;
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

        updateFit();
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


/* TODO

- store per input image settings to .ini file and load that if present
- batch mode, for all entries in a dir's ini file

*/
