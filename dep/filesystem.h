#pragma once

#include <vector>
#include <string>

bool GetFileList(const char *path, std::vector<std::string>& files);
bool GetDirList(const char *path, std::vector<std::string>& files);
bool IsDirectory(const char *s);
void StripFileExtension(std::string& s);
std::string StripFileName(const std::string& s);
std::string Basename(const std::string& s);
bool CreateDir(const char *dir);
bool CreateDirRec(const char *dir);

