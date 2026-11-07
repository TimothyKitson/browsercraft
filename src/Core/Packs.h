#pragma once
#include <string>
#include <vector>

namespace Packs
{
    constexpr int PACK_FORMAT = 34;

    struct Info
    {
        std::string id;
        std::string name;
        std::string description;
        std::string root;
        int format = PACK_FORMAT;
        bool icon = false;
        bool builtIn = false;
    };

    std::string directory(const std::string& configPath);
    std::vector<Info> discover(const std::string& configPath);
    std::string descriptionFrom(const std::string& json);
    int formatFrom(const std::string& json);
    std::vector<std::string> selectionFrom(const std::string& line);
    std::string selectionLine(const std::vector<std::string>& ids);
    std::vector<std::string> savedSelection(const std::string& configPath);
    void setActive(const std::vector<std::string>& roots);
    const std::vector<std::string>& active();
    void activateFromOptions(const std::string& configPath);
    std::string file(const std::string& path);
    unsigned char* load(const char* path, int* width, int* height, int* channels, int wanted);
}
