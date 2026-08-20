#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "Screenshot.h"
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>

void takeScreenshot(int width, int height, const std::string& outputDir) {
    std::vector<uint8_t> pixels(width * height * 3);

    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // flip vertically because OpenGL origin is bottom-left, image origin is top-left
    std::vector<uint8_t> flipped(width * height * 3);
    for (int y = 0; y < height; y++) {
        int srcRow = height - 1 - y;
        memcpy(flipped.data() + y * width * 3,pixels.data() + srcRow * width * 3,width * 3);
    }

    // generate filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time);
    std::ostringstream ss;
    ss << outputDir << "/screenshot_"
        << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << ".png";

    stbi_write_png(ss.str().c_str(), width, height, 3, flipped.data(), width * 3);
    printf("Screenshot saved: %s\n", ss.str().c_str());
}