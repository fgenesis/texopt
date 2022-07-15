#include "util.h"

static bool issep(char x)
{
    return x == DIRSEP || x == '/';
}

static const char localpath[] = { '.', DIRSEP, 0 };

std::string dirname(const std::string & full)
{
    for(size_t i = full.size(); i --> 0;)
    {
        if(issep(full[i]))
            return full.substr(0, i+1); // this includes the '/'
    }
    return &localpath[0];
}

std::string filename(const std::string & full)
{
    for(size_t i = full.size(); i --> 0;)
    {
        if(issep(full[i]))
            return full.substr(i+1);
    }
    return full;
}
