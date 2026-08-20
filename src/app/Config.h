#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ConfigControlPoint {
    float pos, opacity, r, g, b;
};

struct AppConfig {
    std::string lastVolumePath;
    float smoothSigma = 0.0f;
    int renderMode = 0;
    float isoValue = 0.5f;
    bool clipEnabled = false;
    int clipAxis = 0;
    float clipOffset = 0.0f;
    std::vector<ConfigControlPoint> tfPoints;
};

void saveConfig(const std::string& path, const AppConfig& config);
bool loadConfig(const std::string& path, AppConfig& config);