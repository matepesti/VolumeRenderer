#pragma once
#include <string>
#include <filesystem>

#define NOMINMAX
#include <windows.h>

inline std::string getExecutableDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path().string();
}