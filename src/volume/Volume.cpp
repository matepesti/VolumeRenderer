#include "Volume.h"
#include <algorithm>

bool Volume::loadMRC(const std::string& path) {
	FILE* f = nullptr;
	fopen_s(&f, path.c_str(), "rb");
	if (!f) {
		std::perror("Data file couldn't be opened");
		return false;
	}

	char header[1024] = {};
	fread(&header, 1, sizeof(header), f);

	memcpy(&nx, header + 0, sizeof(int32_t));
	memcpy(&ny, header + 4, sizeof(int32_t));
	memcpy(&nz, header + 8, sizeof(int32_t));

	memcpy(&mode, header + 12, sizeof(int32_t));

	float cellX = 0.0f, cellY = 0.0f, cellZ = 0.0f;
	memcpy(&cellX, header + 40, sizeof(float));
	memcpy(&cellY, header + 44, sizeof(float));
	memcpy(&cellZ, header + 48, sizeof(float));

	float dmin = 0.0f, dmax = 0.0f;
	memcpy(&dmin, header + 76, sizeof(float));
	memcpy(&dmax, header + 80, sizeof(float));

	memcpy(&nsymbt, header + 92, sizeof(int32_t));

	spacingX = cellX / nx;
	spacingY = cellY / ny;
	spacingZ = cellZ / nz;

	auto max3 = [&](float a, float b, float c) -> float {
		if (a >= b) {
			if (a >= c)
				return a;
			else
				return c;
		}
		else {
			if (b >= c)
				return b;
			else
				return c;
		}
	};

	float maxDimension = max3(nx * spacingX, ny * spacingY, nz * spacingZ);
	normalizedSizeX = (nx * spacingX) / maxDimension;
	normalizedSizeY = (ny * spacingY) / maxDimension;
	normalizedSizeZ = (nz * spacingZ) / maxDimension;

	minValue = dmin;
	maxValue = dmax;

	std::fseek(f, 1024 + nsymbt, SEEK_SET);

	totalVoxels = nx * ny * nz;

	data.resize(totalVoxels);

	if (!normalizeAndUpload(mode, f, false)) {
		std::perror("Wrong data file or data wrongly read in");
		return false;
	}
	std::fclose(f);
	return true;
}

bool Volume::loadNIfTI(const std::string& path) {
	FILE* f = nullptr;
	fopen_s(&f, path.c_str(), "rb");
	if (!f) {
		std::perror("Data file couldn't be opened");
		return false;
	}

	char header[348] = {};
	fread(&header, 1, sizeof(header), f);

	short dim[8] = {0};

	memcpy(dim, header + 40, sizeof(short) * 8);

	short dataType = 0;

	memcpy(&dataType, header + 70, sizeof(short));

	float pixdim[8] = {};

	memcpy(pixdim, header + 76, sizeof(float) * 8);

	nx = dim[1];
	ny = dim[2];
	nz = dim[3];

	spacingX = pixdim[1];
	spacingY = pixdim[2];
	spacingZ = pixdim[3];

	float maxDimension = std::max({ nx * spacingX, ny * spacingY, nz * spacingZ });
	normalizedSizeX = (nx * spacingX) / maxDimension;
	normalizedSizeY = (ny * spacingY) / maxDimension;
	normalizedSizeZ = (nz * spacingZ) / maxDimension;

	float vox_offset = 0.0f;

	memcpy(&vox_offset, header + 108, sizeof(float));

	std::fseek(f, (long)vox_offset, SEEK_SET);

	totalVoxels = dim[1];
	for (int i = 2; i <= dim[0]; i++) {
		totalVoxels *= dim[i];
	}

	data.resize(totalVoxels);

	if (!normalizeAndUpload(dataType, f, true)) {
		std::perror("Wrong data file or data wrongly read in");
		return false;
	}
	std::fclose(f);
	return true;

}


void Volume::uploadToGPU() {
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_3D, textureID);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32F, nx, ny, nz);
	glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, GL_RED, GL_FLOAT, data.data());
}

void Volume::bind(int textureUnit) {
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_3D, textureID);
}

void Volume::release() {
	if (textureID != 0) {
		glDeleteTextures(1, &textureID);
		textureID = 0;
	}
}

bool Volume::load(const std::string& path) {
	auto endsWith = [](const std::string& str, const std::string& suffix) -> bool {
		if (suffix.size() > str.size()) return false;
		return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
		};

	if (endsWith(path, ".mrc") || endsWith(path, ".map")) {
		return loadMRC(path);
	}
	if (endsWith(path, ".nii.gz")) {
		isCompressed = true;
		return loadNIfTI(path);
	}
	if (endsWith(path, ".nii")) {
		isCompressed = false;
		return loadNIfTI(path);
	}

	std::cout << "Unsupported file format: " << path << "\n";
	return false;
}

// ext = false for map/mrc
// ext = true for nii/nii.gz
bool Volume::normalizeAndUpload(int mode, FILE* f, bool ext) {
	auto normalize = [&](float value, float dmin, float dmax) -> float {
		if (dmax == dmin) return 0.0f;
		float result = (value - dmin) / (dmax - dmin);
		return std::clamp(result, 0.0f, 1.0f);
		};

	if (!ext) {
		if (mode == 0) {
			std::vector<int8_t> raw(totalVoxels);
			std::fread(raw.data(), sizeof(std::int8_t), totalVoxels, f);
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else if (mode == 1) {
			std::vector<int16_t> raw(totalVoxels);
			std::fread(raw.data(), sizeof(std::int16_t), totalVoxels, f);
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else if (mode == 2) {
			std::fread(data.data(), sizeof(float), totalVoxels, f);
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize(data[i], minValue, maxValue);
			}
		}
		else if (mode == 6) {
			std::vector<uint16_t> raw(totalVoxels);
			std::fread(raw.data(), sizeof(std::uint16_t), totalVoxels, f);
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else {
			return false;
		}
	}
	else {
		if (mode == 2) {
			std::vector<int8_t> raw(totalVoxels);
			std::fread(raw.data(), sizeof(std::int8_t), totalVoxels, f);
			maxValue = *std::max_element(raw.begin(), raw.end());
			minValue = *std::min_element(raw.begin(), raw.end());
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else if (mode == 4) {
			std::vector<int16_t> raw(totalVoxels);
			std::fread(raw.data(), sizeof(std::int16_t), totalVoxels, f);
			maxValue = *std::max_element(raw.begin(), raw.end());
			minValue = *std::min_element(raw.begin(), raw.end());
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else if (mode == 8) {
			std::vector<int32_t> raw(totalVoxels);
			std::fread(raw.data(), sizeof(std::int32_t), totalVoxels, f);
			maxValue = *std::max_element(raw.begin(), raw.end());
			minValue = *std::min_element(raw.begin(), raw.end());
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else if (mode == 16) {
			std::fread(data.data(), sizeof(float), totalVoxels, f);
			maxValue = *std::max_element(data.begin(), data.end());
			minValue = *std::min_element(data.begin(), data.end());

			if (!std::isfinite(minValue)) minValue = 0.0f;
			if (!std::isfinite(maxValue)) maxValue = 1.0f;

			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize(data[i], minValue, maxValue);
			}
		}
		else if (mode == 64) {
			std::vector<double> raw(totalVoxels);
			std::fread(raw.data(), sizeof(double), totalVoxels, f);
			maxValue = *std::max_element(raw.begin(), raw.end());
			minValue = *std::min_element(raw.begin(), raw.end());
			for (size_t i = 0; i < data.size(); i++) {
				data[i] = normalize((float)raw[i], minValue, maxValue);
			}
		}
		else {
			return false;
		}

	}

	return true;
}

