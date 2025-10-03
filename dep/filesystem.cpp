#include "filesystem.h"

#if _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <sys/dir.h>
#   include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

static void MakeSlashTerminated(std::string& s)
{
    if(s.length() && s[s.length() - 1] != '/')
        s += '/';
}

#if !_WIN32
static bool _IsFile(const char *path, dirent *dp)
{
    switch(dp->d_type)
    {
        case DT_DIR:
            return false;
        case DT_LNK:
        {
            std::string fullname = path;
            fullname += '/';
            fullname += dp->d_name;
            struct stat statbuf;
            if(stat(fullname.c_str(), &statbuf))
                return false; // error
            return !S_ISDIR(statbuf.st_mode);
        }
    }
    return true;
}
static bool _IsDir(const char *path, dirent *dp)
{
    switch(dp->d_type)
    {
    case DT_DIR:
        return true;
    case DT_LNK:
        {
            std::string fullname = path;
            fullname += '/';
            fullname += dp->d_name;
            struct stat statbuf;
            if(stat(fullname.c_str(), &statbuf))
                return false; // error
            return S_ISDIR(statbuf.st_mode);
        }
    }
    return true;
}
#endif

bool IsDirectory(const char *s)
{
#if _WIN32
    DWORD dwFileAttr = GetFileAttributes(s);
    if(dwFileAttr == INVALID_FILE_ATTRIBUTES)
        return false;
    return !!(dwFileAttr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    if(stat(s, &st))
        return false;

    switch(st->d_type)
    {
        case DT_DIR:
            return true;
        case DT_LNK:
            {
                std::string fullname = path;
                fullname += '/';
                fullname += dp->d_name;
                struct stat statbuf;
                if(stat(fullname.c_str(), &statbuf))
                    return false; // error
                return S_ISDIR(statbuf.st_mode);
            }
    }
    return false;
#endif
}

// returns list of *plain* file names in given directory,
// without subdirs, and without anything else
bool GetFileList(const char *path, std::vector<std::string>& files)
{
#if !_WIN32
    DIR *dirp = opendir(path);
    if(!dirp)
        return false;
    dirent *dp;
    while((dp=readdir(dirp)) != NULL)
        if (_IsFile(path, dp)) // only add if it is not a directory
            files.push_back(dp->d_name);
    closedir(dirp);
    return true;

# else

    WIN32_FIND_DATA fil;
    std::string search(path);
    MakeSlashTerminated(search);
    search += "*";
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        if(!(fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            files.push_back(fil.cFileName);
        }
    }
    while(FindNextFile(hFil, &fil));

    FindClose(hFil);
    return true;

# endif
}

bool GetDirList(const char *path, std::vector<std::string>& files)
{
#if !_WIN32
    DIR *dirp = opendir(path);
    if(!dirp)
        return false;
    dirent *dp;
    while((dp=readdir(dirp)) != NULL)
        if (dp->d_name[0] != '.' && _IsDir(path, dp)) // only add if it is a directory
            files.push_back(dp->d_name);
    closedir(dirp);
    return true;

# else

    WIN32_FIND_DATA fil;
    std::string search(path);
    MakeSlashTerminated(search);
    search += "*";
    HANDLE hFil = FindFirstFile(search.c_str(),&fil);
    if(hFil == INVALID_HANDLE_VALUE)
        return false;
    do
    {
        if(fil.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && fil.cFileName[0] != '.')
        {
            files.push_back(fil.cFileName);
        }
    }
    while(FindNextFile(hFil, &fil));

    FindClose(hFil);
    return true;

# endif
}

void StripFileExtension(std::string& s)
{
    size_t pos = s.find_last_of('.');
    size_t pos2 = s.find_last_of('/');
    if(pos != std::string::npos && (pos2 < pos || pos2 == std::string::npos))
        s.resize(pos);
}

std::string Basename(std::string& s)
{
    size_t pos = s.find_last_of("/\\");
    if(pos == std::string::npos)
        return s;

    return s.substr(pos + 1);
}

std::string StripFileName(const std::string& s)
{
    size_t pos = s.find_last_of("/\\");
    if(pos == std::string::npos)
        return "";

    return s.substr(0, pos);
}

// must return true if creating the directory was successful, or already exists
bool CreateDir(const char *dir)
{
    if(IsDirectory(dir)) // do not try to create if it already exists
        return true;
    bool result;
# if _WIN32
    result = !!::CreateDirectory(dir, NULL);
# else
    result = !mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
    return result;
}

template <class T>
static void StrSplit(const std::string &src, const std::string &sep, T& container, bool keepEmpty = false)
{
    std::string s;
    for (std::string::const_iterator i = src.begin(); i != src.end(); i++)
    {
        if (sep.find(*i) != std::string::npos)
        {
            if (keepEmpty || s.length())
                container.push_back(s);
            s = "";
        }
        else
        {
            s += *i;
        }
    }
    if (keepEmpty || s.length())
        container.push_back(s);
}


bool CreateDirRec(const char *dir)
{
    if(IsDirectory(dir))
        return true;
    bool result = true;
    std::vector<std::string> li;
    StrSplit(dir, "/\\", li, false);
    std::string d;
    d.reserve(strlen(dir) + 1);
    if(*dir == '/')
        d += '/';
    bool last = false;
    for(std::vector<std::string>::iterator it = li.begin(); it != li.end(); ++it)
    {
        d += *it;
        last = CreateDir(d.c_str());
        result = last && result;
        d += '/';
    }
    return result || last;
}
