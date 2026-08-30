#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

class Volume {
private:
	int32_t nx = 0;
	int32_t ny = 0;
	int32_t nz = 0;
	float spacingX = 1.0f;
	float spacingY = 1.0f;
	float spacingZ = 1.0f;
	float minValue = 0.0f;
	float maxValue = 1.0f;
	float normalizedSizeX = 1.0f;
	float normalizedSizeY = 1.0f;
	float normalizedSizeZ = 1.0f;
	std::vector<float> data;
	GLuint textureID = 0;
	int32_t mode = 0;
	int32_t nsymbt = 0;
	bool isCompressed = false;
	int totalVoxels = 0;

public:
	Volume() = default;

	// copying an OpenGL texture would be unsafe
	Volume(const Volume&) = delete;
	Volume& operator=(const Volume&) = delete;

	// Moving ownership of the OpenGL texture
	Volume(Volume&& other) noexcept;
	Volume& operator=(Volume&& other) noexcept;

	~Volume();

	const int32_t getNx() const { return nx; }
	const int32_t getNy() const { return ny; }
	const int32_t getNz() const { return nz; }
	const float getSpacingX() const { return spacingX; }
	const float getSpacingY() const { return spacingY; }
	const float getSpacingZ() const { return spacingZ; }
	const GLuint getTextureID() const { return textureID; }
	const int32_t getMode() const { return mode; }
	const int32_t getNSYMBT() const { return nsymbt; }
	const float getMinValue() const { return minValue; }
	const float getMaxValue() const { return maxValue; }

	bool loadMRC(const std::string& path);
	bool loadNIfTI(const std::string& path);
	bool load(const std::string& path);
	void uploadToGPU();
	void bind(int textureUnit);
	void release();

	const glm::vec3 getVoxelSpacing() const { return glm::vec3(spacingX, spacingY, spacingZ); }
	const glm::ivec3 getDimensions() const { return glm::ivec3(nx, ny, nz); };
	const glm::vec3 getNormalizedSize() const { return glm::vec3(normalizedSizeX,normalizedSizeY,normalizedSizeZ); }

	const std::vector<float>& getData() { return data; }

	bool isValid() const {
		return (textureID != 0 && nx > 0 && ny > 0 && nz > 0 && !data.empty());
	}
};
