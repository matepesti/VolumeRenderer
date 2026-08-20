#pragma once
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "renderer/VolumeRenderer.h"
#include "Config.h"
#include "ui/TransferFunction.h"
struct GLFWwindow;  // forward declare so no need to include glfw here

class App{
public:
    App(int width, int height, const char* title);
    ~App();
    void run();

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    const char* m_title;
    bool m_running = true;
    float m_frameTimeMs;
    bool m_shiftClipActive = false;
    bool m_shiftClipDragging = false;
    float m_shiftClipLastY = 0.0f;

    std::string m_lastLoadedPath = "";

    Camera m_camera;

    bool m_mousePressed = false;
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;

    Volume m_volume;
    VolumeRenderer m_volumeRenderer;
    AppConfig m_config;
    std::string m_configPath;

private:
    void init();
    void mainLoop();
    void drawUI();
    void shutdown();

    void onMouseMove(double x, double y);
    void onScroll(double xoffset, double yoffset);
    void onFramebufferResize(int width, int height);
    void initShiftClip(float mouseX, float mouseY);

    static void s_mouseMoveCallback(GLFWwindow* w, double x, double y);
    static void s_scrollCallback(GLFWwindow* w, double xoff, double yoff);
    static void s_framebufferSizeCallback(GLFWwindow* w, int width, int height);
};