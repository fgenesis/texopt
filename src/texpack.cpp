#include "filesystem.h"
#include "atlas.h"


static void doDir(Atlas& atlas, const char *path)
{
    std::vector<std::string> files;
    std::string dir;
    if(!GetFileList(path, files))
        return;

    std::string pathstr = path;
    pathstr += '/';

    std::string tmp;
    for(size_t i = 0; i < files.size(); ++i)
    {
        tmp = pathstr;
        tmp += files[i];
        atlas.addFile(tmp.c_str());
    }

    atlas.build();
}


int main(int argc, char *argv[])
{
    Atlas atlas;
    atlas.resize(2048, 1024);
    //atlas.updateDistanceMapInterval = 5;

    /*doOneImage("gear.png");
    doOneImage("face.png");
    doOneImage("menu-frame2.png");
    doOneImage("pyramid-dragon-bg.png");
    doOneImage("tentacles.png");
    doOneImage("rock0002.png");
    doOneImage("sunstatue-0001.png");
    doOneImage("city-stairs.png");
    doOneImage("head.png");
    doOneImage("bg-rock-0002.png");
    doOneImage("growth-0004.png");
    doOneImage("pipes-0002.png");
    doOneImage("deep-serpent-body-0001.png");
    doOneImage("coral-0009.png");
    doOneImage("city-tree.png");
    doOneImage("city-tree.png");
    doOneImage("temple-statue-0001.png");
    doOneImage(atlas, "songdoor-glow.png");
    doOneImage(atlas, "mithalas-house-0009.png");
    doOneImage(atlas, "mithal-statue-0010.png");
    doOneImage(atlas, "gazebo-0001.png");
    doOneImage(atlas, "songbubbles.png");
    doOneImage(atlas, "slide-0015.png");*/

    //atlas.addFile("bg-rock-0002.png");

    doDir(atlas, "naija");


    printf("Exiting.\n");
    return 0;
}
