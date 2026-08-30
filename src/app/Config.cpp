#include "Config.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

void saveConfig(const std::string& path,const AppConfig& config) {
    std::ofstream output(path);

    if (!output.is_open()) {
        std::cerr << "Failed to save config: " << path << '\n';
        return;
    }

    output << "smoothSigma=" << std::fixed << std::setprecision(6) << config.smoothSigma << '\n';
    output << "renderMode=" << config.renderMode << '\n';
    output << "isoValue=" << std::fixed << std::setprecision(6) << config.isoValue << '\n';
    output << "clipEnabled=" << config.clipEnabled << '\n';
    output << "clipAxis=" << config.clipAxis << '\n';
    output << "clipOffset=" << std::fixed << std::setprecision(6) << config.clipOffset << '\n';
    output << "tfPointCount=" << config.tfPoints.size() << '\n';

    for (size_t i = 0; i < config.tfPoints.size(); ++i){
        const auto& point = config.tfPoints[i];

        output
            << "tfPoint"
            << i
            << "="
            << std::fixed
            << std::setprecision(6)
            << point.pos << ' '
            << point.opacity << ' '
            << point.r << ' '
            << point.g << ' '
            << point.b
            << '\n';
    }
}

bool loadConfig(const std::string& path,AppConfig& config) {
    std::ifstream input(path);

    if (!input.is_open()) {
        return false;
    }

    config.tfPoints.clear();

    std::string line;

    while (std::getline(input, line)) {
        if (line.empty())
            continue;

        const size_t eq = line.find('=');

        if (eq == std::string::npos)
            continue;

        const std::string key = line.substr(0, eq);

        const std::string value = line.substr(eq + 1);

        try {
            if (key == "smoothSigma") {
                config.smoothSigma = std::stof(value);
            }
            else if (key == "renderMode") {
                config.renderMode = std::stoi(value);
            }
            else if (key == "isoValue") {
                config.isoValue = std::stof(value);
            }
            else if (key == "clipEnabled") {
                config.clipEnabled = std::stoi(value) != 0;
            }
            else if (key == "clipAxis") {
                config.clipAxis = std::stoi(value);
            }
            else if (key == "clipOffset") {
                config.clipOffset = std::stof(value);
            }
            else if (key == "tfPointCount") {
                const int count = std::stoi(value);

                for (int i = 0; i < count; ++i) {
                    if (!std::getline(input, line))
                        break;

                    const size_t pointEq = line.find('=');

                    if (pointEq == std::string::npos)
                        continue;

                    const std::string pointValue = line.substr(pointEq + 1);

                    std::istringstream stream(pointValue);
                    ConfigControlPoint point{};

                    if (stream >> point.pos >> point.opacity >> point.r >> point.g >> point.b) {
                        config.tfPoints.push_back(point);
                    }
                }
            }
        }
        catch (const std::exception&) {
            std::cerr << "Invalid configuration value for key: " << key << '\n';
        }
    }

    return true;
}