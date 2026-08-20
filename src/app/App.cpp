#include "App.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <nfd.h>
#include "Utils.h"
#include "Screenshot.h"

void App::s_framebufferSizeCallback(GLFWwindow* w, int width, int height) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    app->onFramebufferResize(width, height);
}

void App::s_mouseMoveCallback(GLFWwindow* w, double x, double y) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    app->onMouseMove(x, y);
}

void App::s_scrollCallback(GLFWwindow* w, double xoff, double yoff) {
    auto* app = static_cast<App*>(glfwGetWindowUserPointer(w));
    app->onScroll(xoff, yoff);
}

void App::onFramebufferResize(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, width, height);
    m_camera.onWindowResize(width, height);
}

void App::onMouseMove(double x, double y) {
    // only rotate when left mouse button is held

    float dx = static_cast<float>(x) - m_lastMouseX;
    float dy = static_cast<float>(y) - m_lastMouseY;

    if (m_mousePressed && !ImGui::GetIO().WantCaptureMouse) {
        bool isShiftHeld = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
        if (isShiftHeld) {
            if (!m_shiftClipDragging) {
                if (!m_volumeRenderer.isClipEnabled()) {
                    initShiftClip(static_cast<float>(x), static_cast<float>(y));
                }
                m_shiftClipDragging = true;
            }
            else {
                float worldDelta = dy * 0.0025f;
                m_volumeRenderer.offsetClip(worldDelta);
            }
        }
        else {
            m_shiftClipDragging = false;
            m_camera.onMouseDrag(dx, dy);
        }
    }
    if (!m_mousePressed) m_shiftClipDragging = false;

    m_lastMouseX = static_cast<float>(x);
    m_lastMouseY = static_cast<float>(y);
}

void App::onScroll(double xoffset, double yoffset) {
    if (!ImGui::GetIO().WantCaptureMouse)
        m_camera.onScroll(static_cast<float>(yoffset));
}

void App::initShiftClip(float mouseX, float mouseY) {
    // convert to NDC
    float ndcX = (mouseX / m_width) * 2.0f - 1.0f;
    float ndcY = 1.0f - (mouseY / m_height) * 2.0f;

    // reconstruct ray
    glm::vec4 clipPos = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 viewPos = m_camera.getInverseProjectionMatrix() * clipPos;
    viewPos = glm::vec4(glm::vec2(viewPos) / viewPos.w, -1.0f, 0.0f);
    glm::vec4 worldDir = m_camera.getInverseViewMatrix() * viewPos;
    glm::vec3 rayDir = glm::normalize(glm::vec3(worldDir));
    glm::vec3 rayOrigin = m_camera.getPosition();

    // slab test against volume bounding box
    glm::vec3 halfSize = m_volume.getNormalizedSize() * 0.5f;
    glm::vec3 tMin = (-halfSize - rayOrigin) / rayDir;
    glm::vec3 tMax = (halfSize - rayOrigin) / rayDir;
    glm::vec3 t1 = glm::min(tMin, tMax);
    glm::vec3 t2 = glm::max(tMin, tMax);
    float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);

    if (tFar < 0.0f || tNear > tFar) return;  // ray missed volume

    // hit point on the volume
    glm::vec3 hitPoint = rayOrigin + tNear * rayDir;

    // clip plane normal = camera forward direction
    glm::vec3 clipNormal = glm::normalize(m_camera.getPosition() - glm::vec3(0.0f));
    float clipOffset = -glm::dot(hitPoint, clipNormal);

    m_volumeRenderer.setClipPlane(clipNormal, clipOffset);
    m_volumeRenderer.setClipEnabled(true);
}

App::App(int width, int height, const char* title): m_width(width), m_height(height), m_title(title), m_camera(120.0f, glm::vec3(0.0f), 60.0f){
    init();
}

App::~App() {
    shutdown();
}

void App::run() {
    mainLoop();
}

void App::init() {
    // initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    // tell GLFW what context we want before creating the window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // create the window
    m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    // make this window's OpenGL context active on this thread
    glfwMakeContextCurrent(m_window);

    // enable vsync
    glfwSwapInterval(1);

    // load OpenGL function pointers via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GPU: " << glGetString(GL_RENDERER) << std::endl;

    // set initial viewport
    glViewport(0, 0, m_width, m_height);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetCursorPosCallback(m_window, s_mouseMoveCallback);
    glfwSetScrollCallback(m_window, s_scrollCallback);

    if (m_volume.load("C:/Users/Pesti Máté/source/repos/VolumeRenderer/data/emd_11457.map")) {
        m_volume.uploadToGPU();

        printf("Loaded: %d x %d x %d, spacing %.3f %.3f %.3f\n",m_volume.getNx(), m_volume.getNy(), m_volume.getNz(),m_volume.getSpacingX(), m_volume.getSpacingY(), m_volume.getSpacingZ());
    }
    else {
        std::cout << "Failed to load volume" << "\n";

    }
    std::string exeDir = getExecutableDir();
    m_volumeRenderer.init(exeDir);

    // register resize callback
    glfwSetFramebufferSizeCallback(m_window, s_framebufferSizeCallback);

    // initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGuiIO& io = ImGui::GetIO();
    // enable docking and keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // dark theme
    ImGui::StyleColorsDark();

    // bind ImGui to our GLFW window and OpenGL 4.5
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    NFD_Init();


    m_configPath = getExecutableDir() + "/volumerenderer.cfg";
    AppConfig loaded;
    if (loadConfig(m_configPath, loaded)) {
        m_config = loaded;
        if (!m_config.lastVolumePath.empty()) {
            if (m_volume.load(m_config.lastVolumePath)) {
                m_volume.uploadToGPU();
                m_lastLoadedPath = m_config.lastVolumePath;
                m_volumeRenderer.getTransferFunction().computeHistogram(m_volume.getData());
            }
        }
        m_volumeRenderer.setSmoothSigma(m_config.smoothSigma);
        m_volumeRenderer.setRenderMode(m_config.renderMode);
        m_volumeRenderer.setIsoValue(m_config.isoValue);
        m_volumeRenderer.setClipOffset(m_config.clipOffset);
        m_volumeRenderer.setClipEnabled(m_config.clipEnabled);
        m_volumeRenderer.setClipAxis(m_config.clipAxis);
        if (!m_config.tfPoints.empty()) {
            TransferFunction tf = m_volumeRenderer.getTransferFunction();
            std::vector<ControlPoint> cpArray = tf.getControlPoints();
            for (int i = 0; i < m_config.tfPoints.size(); i++) {
                ControlPoint cp;
                cp.pos = m_config.tfPoints[i].pos;
                cp.opacity = m_config.tfPoints[i].opacity;
                float r = m_config.tfPoints[i].r;
                float g = m_config.tfPoints[i].g;
                float b = m_config.tfPoints[i].b;
                cp.color = glm::vec3(r,g,b);
                cpArray.push_back(cp);
            }
        }
    }
}

void App::mainLoop() {
    while (!glfwWindowShouldClose(m_window) && m_running)
    {
        // poll all pending input events
        glfwPollEvents();
        m_mousePressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // begin ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // build the UI for this frame
        drawUI();

        // frame timing
        double frameStart = glfwGetTime();

        // OpenGL rendering
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_volumeRenderer.render(m_camera, m_volume);

        // render ImGui on top
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap front and back buffers to show the frame
        glfwSwapBuffers(m_window);

        double frameEnd = glfwGetTime();
        double frameTime = (frameEnd - frameStart) * 1000; // ms
        m_frameTimeMs = (float)frameTime;
    }
}

void App::drawUI()
{
    // fullscreen dockspace so panels can be docked
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("DockSpace", nullptr, dockFlags);
    ImGui::PopStyleVar();

    ImGuiID dockID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();

    ImGui::Begin("Controls");
    ImGui::Text("Volume Renderer");
    ImGui::Separator();

    if (ImGui::Button("Open Volume...")) {
        nfdchar_t* outPath = nullptr;
        nfdfilteritem_t filters[2] = {
            { "Volume files", "mrc,map,nii" },
            { "All files", "*" }
        };
        nfdresult_t result = NFD_OpenDialog(&outPath, filters, 2, nullptr);

        if (result == NFD_OKAY) {
            std::string path(outPath);
            NFD_FreePath(outPath);

            // reload the volume
            m_volume.release();
            if (m_volume.load(path)) {
                m_volume.uploadToGPU();
                m_volumeRenderer.resetSmoothing();
                m_lastLoadedPath = path;
                printf("Loaded: %s\n", path.c_str());
            }
            else {
                printf("Failed to load: %s\n", path.c_str());
            }
        }
    }

    if (ImGui::Button("Export PNG")) {
        takeScreenshot(m_width, m_height, getExecutableDir());
    }

    ImGui::Separator();
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Frame time: %.2f ms", m_frameTimeMs);
    ImGui::Separator();
    glm::vec3 pos = m_camera.getPosition();
    ImGui::Text("Camera pos: %.2f %.2f %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Render Mode");
    m_volumeRenderer.drawRenderControls(m_camera);
    if (ImGui::Button("Quit"))
        m_running = false;
    m_volumeRenderer.getTransferFunction().drawUI();
    ImGui::End();
}

void App::shutdown()
{
    m_config.lastVolumePath = m_lastLoadedPath;
    m_config.smoothSigma = m_volumeRenderer.getSmoothSigma();
    m_config.renderMode = m_volumeRenderer.getRenderMode();
    m_config.isoValue = m_volumeRenderer.getIsoValue();
    m_config.clipOffset = m_volumeRenderer.getClipOffset();
    m_config.clipEnabled = m_volumeRenderer.getClipEnabled();
    m_config.clipAxis = m_volumeRenderer.getClipAxis();
    auto& tfPoints = m_volumeRenderer.getTransferFunction().getControlPoints();
    m_config.tfPoints.clear();
    for (auto& pt : tfPoints) {
        m_config.tfPoints.push_back({ pt.pos, pt.opacity, pt.color.r, pt.color.g, pt.color.b });
    }
    saveConfig(m_configPath, m_config);


    NFD_Quit();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}