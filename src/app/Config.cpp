#include "Config.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

void saveConfig(const std::string& path, const AppConfig& config) {
	std::ofstream output;
	output.open(path);

	output << "lastVolumePath=" << config.lastVolumePath << "\n";
	output << "smoothSigma=" << std::fixed << std::setprecision(6) << config.smoothSigma << "\n";
	output << "renderMode=" << config.renderMode << "\n";
	output << "isoValue=" << std::fixed << std::setprecision(6) << config.isoValue << "\n";
	output << "clipEnabled=" << config.clipEnabled << "\n";
	output << "clipAxis=" << config.clipAxis<< "\n";
	output << "clipOffset=" << std::fixed << std::setprecision(6) << config.clipOffset<< "\n";
	int pointsSize = config.tfPoints.size();
	output << "tfPointCount=" << pointsSize << "\n";
	for (int i = 0; i < pointsSize; i++) {
		output << "tfPoint" << i << "=" << std::fixed << std::setprecision(6) <<
			config.tfPoints[i].pos << " " <<
			config.tfPoints[i].opacity << " " <<
			config.tfPoints[i].r << " " <<
			config.tfPoints[i].g << " " <<
			config.tfPoints[i].b << " " << "\n";
	}

	output.close();
}

bool loadConfig(const std::string& path, AppConfig& config) {
	std::ifstream input;
	input.open(path, 'r');
	if (!input.is_open()) {
		return false;
	}
	std::string line;
	
	while (std::getline(input, line)) {
		size_t indexOfEq = line.find('=');
		std::string key = line.substr(0, indexOfEq);
		std::string value = line.substr(indexOfEq + 1);

		if (key == "lastVolumePath") {
			config.lastVolumePath = value;
		}
		else if (key == "smoothSigma") {
			float fValue = std::stof(value);
			config.smoothSigma = fValue;
		}
		else if (key == "renderMode") {
			int iValue = std::stoi(value);
			config.renderMode = iValue;
		}
		else if (key == "isoValue") {
			float fValue = std::stof(value);
			config.isoValue = fValue;
		}
		else if (key == "clipEnabled") {
			int iValue = std::stoi(value);
			config.clipEnabled = iValue;
		}
		else if (key == "clipAxis") {
			int iValue = std::stoi(value);
			config.clipAxis = iValue;
		}
		else if (key == "clipOffset") {
			float fValue = std::stof(value);
			config.clipOffset = fValue;
		}
		else if (key == "tfPointCount") {
			int iValue = std::stoi(value);
			for (int i = 0; i < iValue; i++) {
				std::getline(input, line);
				indexOfEq = line.find('=');
				key = line.substr(0, indexOfEq);
				value = line.substr(indexOfEq + 1);
				std::istringstream stringstr(value);
				ConfigControlPoint ccp;
				stringstr >> ccp.pos >> ccp.opacity >> ccp.r >> ccp.g >> ccp.b;
				config.tfPoints.push_back(ccp);
			}
		}
		else {
			std::cout << "Wrong config key" << "\n";
		}
	}
	return true;
}