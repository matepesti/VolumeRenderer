#include "Volume.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>

Volume::Volume(Volume&& other) noexcept
	: nx(other.nx),
	ny(other.ny),
	nz(other.nz),
	spacingX(other.spacingX),
	spacingY(other.spacingY),
	spacingZ(other.spacingZ),
	minValue(other.minValue),
	maxValue(other.maxValue),
	normalizedSizeX(other.normalizedSizeX),
	normalizedSizeY(other.normalizedSizeY),
	normalizedSizeZ(other.normalizedSizeZ),
	data(std::move(other.data)),
	textureID(other.textureID),
	mode(other.mode),
	nsymbt(other.nsymbt),
	totalVoxels(other.totalVoxels) 
{
	other.nx = 0;
	other.ny = 0;
	other.nz = 0;
	other.textureID = 0;
	other.totalVoxels = 0;
}

Volume& Volume::operator=(Volume&& other) noexcept {
	if (this == &other)
		return *this;

	release();

	nx = other.nx;
	ny = other.ny;
	nz = other.nz;

	spacingX = other.spacingX;
	spacingY = other.spacingY;
	spacingZ = other.spacingZ;

	minValue = other.minValue;
	maxValue = other.maxValue;

	normalizedSizeX = other.normalizedSizeX;
	normalizedSizeY = other.normalizedSizeY;
	normalizedSizeZ = other.normalizedSizeZ;

	data = std::move(other.data);

	textureID = other.textureID;
	mode = other.mode;
	nsymbt = other.nsymbt;
	totalVoxels = other.totalVoxels;

	other.nx = 0;
	other.ny = 0;
	other.nz = 0;
	other.textureID = 0;
	other.totalVoxels = 0;

	return *this;
}

Volume::~Volume() {
	release();
}

bool Volume::loadMRC(const std::string& path) {
	FILE* f = nullptr;
	
	if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) {
		std::cerr << "Failed to open MRC/MAP file: " << path << '\n';
		return false;
	}

	char header[1024] = {};

    const size_t headerBytes = fread(header,1,sizeof(header),f);

    if (headerBytes != sizeof(header)) {
        std::cerr << "Invalid MRC/MAP file: header is incomplete.\n";
        fclose(f);
        return false;
    }

    int32_t fileNx = 0;
    int32_t fileNy = 0;
    int32_t fileNz = 0;
    int32_t fileMode = 0;

    memcpy(&fileNx, header + 0, sizeof(int32_t));
    memcpy(&fileNy, header + 4, sizeof(int32_t));
    memcpy(&fileNz, header + 8, sizeof(int32_t));
    memcpy(&fileMode, header + 12, sizeof(int32_t));

    if (fileNx <= 0 || fileNy <= 0 || fileNz <= 0) {
        std::cerr<< "Invalid MRC/MAP dimensions: "<< fileNx << " x "<< fileNy << " x "<< fileNz << '\n';
        fclose(f);
        return false;
    }

    if (fileNx > 16384 || fileNy > 16384 || fileNz > 16384) {
        std::cerr << "Volume dimensions are unreasonably large: "<< fileNx << " x "<< fileNy << " x "<< fileNz << '\n';
        fclose(f);
        return false;
    }

    float cellX = 0.0f;
    float cellY = 0.0f;
    float cellZ = 0.0f;

    memcpy(&cellX, header + 40, sizeof(float));
    memcpy(&cellY, header + 44, sizeof(float));
    memcpy(&cellZ, header + 48, sizeof(float));

    float dmin = 0.0f;
    float dmax = 0.0f;

    memcpy(&dmin, header + 76, sizeof(float));
    memcpy(&dmax, header + 80, sizeof(float));

    int32_t fileNsymbt = 0;
    memcpy(&fileNsymbt, header + 92, sizeof(int32_t));

    std::cout
        << "MRC metadata:\n"
        << "  dimensions: "
        << fileNx << " x "
        << fileNy << " x "
        << fileNz << '\n'
        << "  mode: "
        << fileMode << '\n'
        << "  cell size: "
        << cellX << ", "
        << cellY << ", "
        << cellZ << '\n'
        << "  dmin: "
        << dmin << '\n'
        << "  dmax: "
        << dmax << '\n'
        << "  nsymbt: "
        << fileNsymbt << '\n';

    if (fileNsymbt < 0 || fileNsymbt > 1024 * 1024 * 1024) {
        std::cerr << "Invalid MRC/MAP extended header size.\n";
        fclose(f);
        return false;
    }

    if (cellX <= 0.0f ||cellY <= 0.0f ||cellZ <= 0.0f) {
        std::cerr << "Invalid voxel spacing in MRC/MAP file.\n";
        fclose(f);
        return false;
    }

    const int64_t voxelCount64 = static_cast<int64_t>(fileNx) * static_cast<int64_t>(fileNy) * static_cast<int64_t>(fileNz);

    if (voxelCount64 <= 0 ||voxelCount64 > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        std::cerr << "Volume is too large to load safely.\n";
        fclose(f);
        return false;
    }

    const int fileTotalVoxels = static_cast<int>(voxelCount64);

    if (fseek(f, 1024L + fileNsymbt, SEEK_SET) != 0)
    {
        std::cerr << "Failed to seek to MRC/MAP voxel data.\n";
        fclose(f);
        return false;
    }

    // temporarily store the new state so if loading fails, the existing Volume object is not partially overwritten
    std::vector<float> newData(fileTotalVoxels);

    auto normalize =
        [](float value, float minimum, float maximum) -> float {
            if (maximum <= minimum)
                return 0.0f;

            return std::clamp((value - minimum) / (maximum - minimum),0.0f,1.0f);
        };

    if (fileMode == 0) {
        std::vector<int8_t> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(int8_t),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read MRC/MAP int8 data.\n";
            fclose(f);
            return false;
        }

        for (size_t i = 0; i < newData.size(); ++i) {
            newData[i] = normalize(static_cast<float>(raw[i]),dmin,dmax);
        }
    }
    else if (fileMode == 1) {
        std::vector<int16_t> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(int16_t),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read MRC/MAP int16 data.\n";
            fclose(f);
            return false;
        }

        for (size_t i = 0; i < newData.size(); ++i) {
            newData[i] = normalize(static_cast<float>(raw[i]),dmin,dmax);
        }
    }
    else if (fileMode == 2) {
        std::vector<float> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(float),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read MRC/MAP float32 data.\n";
            fclose(f);
            return false;
        }

        for (size_t i = 0; i < raw.size(); ++i) {
            newData[i] = normalize(raw[i],dmin,dmax);
        }
    }
    else if (fileMode == 6) {
        std::vector<uint16_t> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(uint16_t),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read MRC/MAP uint16 data.\n";
            fclose(f);
            return false;
        }

        for (size_t i = 0; i < newData.size(); ++i)
        {
            newData[i] = normalize(static_cast<float>(raw[i]),dmin,dmax);
        }
    }
    else {
        std::cerr << "Unsupported MRC/MAP mode: " << fileMode << '\n';

        fclose(f);
        return false;
    }

    fclose(f);

    const float physicalX = fileNx * (cellX / fileNx);
    const float physicalY = fileNy * (cellY / fileNy);
    const float physicalZ = fileNz * (cellZ / fileNz);
    const float maxDimension = std::max({physicalX,physicalY,physicalZ});

    if (maxDimension <= 0.0f) {
        std::cerr << "Invalid physical volume dimensions.\n";
        return false;
    }

	// actually loading everything after making sure nothing failed, so the object is not partially overwritten

    nx = fileNx;
    ny = fileNy;
    nz = fileNz;

    spacingX = cellX / fileNx;
    spacingY = cellY / fileNy;
    spacingZ = cellZ / fileNz;

    minValue = dmin;
    maxValue = dmax;

    normalizedSizeX = physicalX / maxDimension;
    normalizedSizeY = physicalY / maxDimension;
    normalizedSizeZ = physicalZ / maxDimension;

    mode = fileMode;
    nsymbt = fileNsymbt;
    totalVoxels = fileTotalVoxels;

    data = std::move(newData);

    std::cout << "First 10 normalized MRC voxel values:\n";

    for (size_t i = 0; i < std::min<size_t>(10, data.size()); ++i) {
        std::cout << "  [" << i << "] = " << data[i] << '\n';
    }

    std::cout << "MRC data size: " << data.size() << " floats\n";
    std::cout << "MRC expected size: " << (static_cast<size_t>(nx) *static_cast<size_t>(ny) *static_cast<size_t>(nz)) << " floats\n";

    return true;
}

bool Volume::loadNIfTI(const std::string& path) {
	FILE* f = nullptr;
	
	if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) {
		std::cerr << "Invalid NIfTI file: header is incomplete.\n";
		return false;
	}

	char header[348] = {};

    if (fread(header, 1, sizeof(header), f) != sizeof(header)) {
        std::cerr << "Invalid NIfTI file: header is incomplete.\n";
        fclose(f);
        return false;
    }

    int32_t sizeofHeader = 0;

    memcpy(&sizeofHeader,header + 0,sizeof(int32_t));

    if (sizeofHeader != 348) {
        std::cerr << "Unsupported or invalid NIfTI header.\n" << "Expected sizeof_hdr = 348, got " << sizeofHeader << '\n'; 
        fclose(f);
        return false;
    }

    int16_t dim[8] = {};

    memcpy(dim,header + 40,sizeof(int16_t) * 8);

    const int16_t ndim = dim[0];

    if (ndim < 3 || ndim > 3) {
        std::cerr << "This renderer currently supports only 3D NIfTI volumes.\n" << "Number of dimensions: " << ndim << '\n'; 
        fclose(f);
        return false;
    }

    const int32_t fileNx = dim[1];
    const int32_t fileNy = dim[2];
    const int32_t fileNz = dim[3];

    if (fileNx <= 0 || fileNy <= 0 || fileNz <= 0) {
        std::cerr << "Invalid NIfTI dimensions: " << fileNx << " x " << fileNy << " x " << fileNz << '\n'; 
        fclose(f);
        return false;
    }

    if (fileNx > 16384 || fileNy > 16384 || fileNz > 16384) {
        std::cerr << "NIfTI volume dimensions are unreasonably large.\n"; 
        fclose(f);
        return false;
    }

    int16_t dataType = 0;
    int16_t bitsPerVoxel = 0;

    memcpy(&dataType,header + 70,sizeof(int16_t));

    memcpy(&bitsPerVoxel,header + 72,sizeof(int16_t));

    float pixdim[8] = {};

    memcpy(pixdim,header + 76,sizeof(float) * 8);

    const float fileSpacingX = std::abs(pixdim[1]);
    const float fileSpacingY = std::abs(pixdim[2]);
    const float fileSpacingZ = std::abs(pixdim[3]);

    if (fileSpacingX <= 0.0f || fileSpacingY <= 0.0f || fileSpacingZ <= 0.0f) {
        std::cerr << "Invalid NIfTI voxel spacing.\n";
        fclose(f);
        return false;
    }

    float voxOffset = 0.0f;

    memcpy(&voxOffset,header + 108,sizeof(float));

    if (!std::isfinite(voxOffset) || voxOffset < 348.0f) {
        std::cerr << "Invalid NIfTI vox_offset: " << voxOffset << '\n'; 
        fclose(f);
        return false;
    }

    const int64_t voxelCount64 = static_cast<int64_t>(fileNx) * static_cast<int64_t>(fileNy) * static_cast<int64_t>(fileNz);

    if (voxelCount64 <= 0 || voxelCount64 > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        std::cerr << "NIfTI volume is too large to load safely.\n";
        fclose(f);
        return false;
    }

    const int fileTotalVoxels = static_cast<int>(voxelCount64);

    if (fseek(f, static_cast<long>(voxOffset), SEEK_SET) != 0) {
        std::cerr << "Failed to seek to NIfTI voxel data.\n";
        fclose(f);
        return false;
    }

    std::vector<float> newData(fileTotalVoxels);

    auto normalize =
        [](float value, float minimum, float maximum) -> float
        {
            if (maximum <= minimum)
                return 0.0f;

            return std::clamp((value - minimum) / (maximum - minimum),0.0f,1.0f);
        };

    float rawMin = std::numeric_limits<float>::max();
    float rawMax = std::numeric_limits<float>::lowest();

    if (dataType == 2) {
        std::vector<uint8_t> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(uint8_t),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read NIfTI uint8 data.\n";
            fclose(f);
            return false;
        }

        for (uint8_t value : raw) {
            rawMin = std::min(rawMin,static_cast<float>(value));
            rawMax = std::max(rawMax,static_cast<float>(value));
        }

        for (size_t i = 0; i < raw.size(); ++i) {
            newData[i] = normalize(static_cast<float>(raw[i]),rawMin,rawMax);
        }
    }
    else if (dataType == 4) {
        std::vector<int16_t> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(int16_t),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read NIfTI int16 data.\n";
            fclose(f);
            return false;
        }

        for (int16_t value : raw) {
            rawMin = std::min(rawMin,static_cast<float>(value));
            rawMax = std::max(rawMax,static_cast<float>(value));
        }

        for (size_t i = 0; i < raw.size(); ++i) {
            newData[i] = normalize(static_cast<float>(raw[i]),rawMin,rawMax);
        }
    }
    else if (dataType == 8) {
        std::vector<int32_t> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(int32_t),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read NIfTI int32 data.\n";
            fclose(f);
            return false;
        }

        for (int32_t value : raw) {
            rawMin = std::min(rawMin,static_cast<float>(value));
            rawMax = std::max(rawMax,static_cast<float>(value));
        }

        for (size_t i = 0; i < raw.size(); ++i) {
            newData[i] = normalize(static_cast<float>(raw[i]),rawMin,rawMax);
        }
    }
    else if (dataType == 16) {
        std::vector<float> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(float),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read NIfTI float32 data.\n";
            fclose(f);
            return false;
        }

        for (float value : raw) {
            if (std::isfinite(value)){
                rawMin = std::min(rawMin, value);
                rawMax = std::max(rawMax, value);
            }
        }

        if (!std::isfinite(rawMin) || !std::isfinite(rawMax) || rawMax <= rawMin) {
            std::cerr << "NIfTI float volume contains no usable intensity range.\n";
            fclose(f);
            return false;
        }

        for (size_t i = 0; i < raw.size(); ++i) {
            float value = raw[i];

            if (!std::isfinite(value))
                value = rawMin;

            newData[i] = normalize(value,rawMin,rawMax);
        }
    }
    else if (dataType == 64)
    {
        std::vector<double> raw(fileTotalVoxels);

        if (fread(raw.data(),sizeof(double),raw.size(),f) != raw.size()) {
            std::cerr << "Failed to read NIfTI float64 data.\n";
            fclose(f);
            return false;
        }

        for (double value : raw) {
            if (std::isfinite(value)) {
                rawMin = std::min(rawMin,static_cast<float>(value));
                rawMax = std::max(rawMax,static_cast<float>(value));
            }
        }

        if (!std::isfinite(rawMin) || !std::isfinite(rawMax) || rawMax <= rawMin){
            std::cerr << "NIfTI float64 volume contains no usable intensity range.\n";
            fclose(f);
            return false;
        }

        for (size_t i = 0; i < raw.size(); ++i) {
            float value = std::isfinite(raw[i]) ? static_cast<float>(raw[i]) : rawMin;
            newData[i] = normalize(value,rawMin,rawMax);
        }
    }
    else {
        std::cerr<< "Unsupported NIfTI datatype: "<< dataType<< " (" << bitsPerVoxel<< " bits/voxel)\n";
        fclose(f);
        return false;
    }

    fclose(f);

    const float physicalX = fileNx * fileSpacingX;
    const float physicalY = fileNy * fileSpacingY;
    const float physicalZ = fileNz * fileSpacingZ;
    const float maxDimension = std::max({physicalX,physicalY,physicalZ});

    if (maxDimension <= 0.0f) {
        std::cerr << "Invalid physical NIfTI dimensions.\n";
        return false;
    }

    // Commit only after complete successful read
    nx = fileNx;
    ny = fileNy;
    nz = fileNz;

    spacingX = fileSpacingX;
    spacingY = fileSpacingY;
    spacingZ = fileSpacingZ;

    minValue = rawMin;
    maxValue = rawMax;

    normalizedSizeX = physicalX / maxDimension;
    normalizedSizeY = physicalY / maxDimension;
    normalizedSizeZ = physicalZ / maxDimension;

    mode = dataType;
    nsymbt = 0;
    totalVoxels = fileTotalVoxels;

    data = std::move(newData);

    return true;

}

void Volume::uploadToGPU()
{
    if (data.empty() || nx <= 0 || ny <= 0 || nz <= 0) {
        std::cerr << "Cannot upload invalid volume to GPU.\n";
        return;
    }

    const size_t voxelCount = static_cast<size_t>(nx) * static_cast<size_t>(ny) * static_cast<size_t>(nz);

    if (data.size() != voxelCount) {
        std::cerr << "Volume data size mismatch.\n" << "Expected: " << voxelCount << " floats\n" << "Actual: " << data.size() << " floats\n";
        return;
    }

    const size_t dataSize = voxelCount * sizeof(float);

    const double memoryMB = static_cast<double>(dataSize) / (1024.0 * 1024.0);

    std::cout << "Uploading volume: " << nx << " x " << ny << " x " << nz << " (" << memoryMB << " MB)\n";

    // clear any stale OpenGL errors
    while (glGetError() != GL_NO_ERROR)
    {
    }

    // explicitly define pixel-unpack state
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,0);

    // make sure no PBO is currently bound
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

    // remove previous texture owned by this volume.
    release();

    // Create 3D texture
    glGenTextures(1,&textureID);

    if (textureID == 0){
        std::cerr << "Failed to create 3D texture.\n";
        return;
    }

    glBindTexture(GL_TEXTURE_3D,textureID);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_3D,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);

    // allocate texture storage
    glTexImage3D(GL_TEXTURE_3D,0,GL_R32F,nx,ny,nz,0,GL_RED,GL_FLOAT,nullptr);

    GLenum error = glGetError();

    if (error != GL_NO_ERROR){
        std::cerr << "Failed to allocate 3D texture. OpenGL error: 0x" << std::hex << error << std::dec << '\n';

        glBindTexture(GL_TEXTURE_3D,0);

        release();

        return;
    }
    // Create PBO

    GLuint pbo = 0;

    glGenBuffers(1,&pbo);

    if (pbo == 0){
        std::cerr << "Failed to create PBO.\n";

        glBindTexture(GL_TEXTURE_3D,0);

        release();

        return;
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER,static_cast<GLsizeiptr>(dataSize),nullptr,GL_STREAM_DRAW);

    error = glGetError();

    if (error != GL_NO_ERROR) {
        std::cerr << "Failed to allocate PBO. OpenGL error: 0x" << std::hex << error << std::dec << '\n';

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
        glDeleteBuffers(1,&pbo);
        glBindTexture(GL_TEXTURE_3D,0);

        release();

        return;
    }

    // CPU -> PBO

    glBufferSubData(GL_PIXEL_UNPACK_BUFFER,0,static_cast<GLsizeiptr>(dataSize),data.data());

    error = glGetError();

    if (error != GL_NO_ERROR){
        std::cerr << "Failed to copy voxel data to PBO. OpenGL error: 0x" << std::hex << error << std::dec << '\n';

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
        glDeleteBuffers(1, &pbo);
        glBindTexture(GL_TEXTURE_3D,0);

        release();

        return;
    }

    // PBO -> 3D texture

    glTexImage3D(GL_TEXTURE_3D,0,GL_R32F,nx,ny,nz,0,GL_RED,GL_FLOAT,reinterpret_cast<const void*>(0));

    error = glGetError();

    if (error != GL_NO_ERROR) {
        std::cerr << "Failed to upload PBO to 3D texture. OpenGL error: 0x" << std::hex << error << std::dec << '\n';

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
        glDeleteBuffers(1,&pbo);
        glBindTexture(GL_TEXTURE_3D,0);

        release();

        return;
    }

    // Cleanup

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
    glDeleteBuffers(1,&pbo);
    glBindTexture(GL_TEXTURE_3D,0);

    std::cout << "Volume uploaded successfully.\n";
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

	if (endsWith(path, ".mrc") || endsWith(path, ".map") || endsWith(path, ".MRC") || endsWith(path, ".MAP")) {
		return loadMRC(path);
	}
    if (endsWith(path, ".nii")|| endsWith(path, ".NII")) {
		isCompressed = false;
		return loadNIfTI(path);
	}

	std::cout << "Unsupported file format: " << path << "\n";
	return false;
}

