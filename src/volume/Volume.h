#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

class Volume {
private:
	int32_t nx;
	int32_t ny;
	int32_t nz;
	float spacingX;
	float spacingY;
	float spacingZ;
	float minValue;
	float maxValue;
	float normalizedSizeX;
	float normalizedSizeY;
	float normalizedSizeZ;
	std::vector<float> data;
	GLuint textureID = 0;
	int32_t mode = 0;
	int32_t nsymbt = 0;
	bool isCompressed = false;
	int totalVoxels = 0;

	bool normalizeAndUpload(int mode, FILE* f,bool ext);


public:
	int32_t getNx() { return nx; }
	int32_t getNy() { return ny; }
	int32_t getNz() { return nz; }
	float getSpacingX() { return spacingX; }
	float getSpacingY() { return spacingY; }
	float getSpacingZ() { return spacingZ; }
	GLuint getTextureID() { return textureID; }
	int32_t getMode() { return mode; }
	int32_t getNSYMBT() { return nsymbt; }
	float getMinValue() { return minValue; }
	float getMaxValue() { return maxValue; }

	bool loadMRC(const std::string& path);
	bool loadNIfTI(const std::string& path);
	bool load(const std::string& path);
	void uploadToGPU();
	void bind(int textureUnit);
	void release();

	glm::vec3 getVoxelSpacing() { return glm::vec3(spacingX, spacingY, spacingZ); }
	glm::ivec3 getDimensions(); // - not implemented yet. (needed ?)
	glm::vec3 getNormalizedSize() { return glm::vec3(normalizedSizeX,normalizedSizeY,normalizedSizeZ); }

	std::vector<float> getData() { return data; }

};
