cmake_minimum_required(VERSION 3.20)
project(VolumeRenderer VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# GLFW (pre-compiled binary)
# -------------------------------------------------------
add_library(glfw IMPORTED STATIC GLOBAL)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "C:/GLFW/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "C:/GLFW/glfw-3.4.bin.WIN64/include"
)

# -------------------------------------------------------
# GLAD (local copy)
# -------------------------------------------------------
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
)

# -------------------------------------------------------
# GLM (header only)
# -------------------------------------------------------
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
    C:/GLM/glm-master
)

# -------------------------------------------------------
# Dear ImGui (docking branch)
# -------------------------------------------------------
add_library(imgui STATIC
    C:/imgui/imgui-docking/imgui.cpp
    C:/imgui/imgui-docking/imgui_draw.cpp
    C:/imgui/imgui-docking/imgui_tables.cpp
    C:/imgui/imgui-docking/imgui_widgets.cpp
    C:/imgui/imgui-docking/backends/imgui_impl_glfw.cpp
    C:/imgui/imgui-docking/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC
    C:/imgui/imgui-docking
    C:/imgui/imgui-docking/backends
    C:/GLFW/glfw-3.4.bin.WIN64/include
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
)

# -------------------------------------------------------
# NFD
# -------------------------------------------------------

add_library(nfd STATIC
    ${CMAKE_SOURCE_DIR}/third_party/nfd/nfd_win.cpp
    #${CMAKE_SOURCE_DIR}/third_party/nfd/nfd_common.cpp
)
target_include_directories(nfd PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/nfd/include
)
target_link_libraries(nfd PRIVATE comctl32 ole32 uuid)

# -------------------------------------------------------
# VolumeRenderer executable
# -------------------------------------------------------
add_executable(VolumeRenderer
    src/main.cpp
    src/app/App.h
    src/app/App.cpp
 "src/camera/Camera.h" "src/camera/Camera.cpp" "src/volume/Volume.h" "src/volume/Volume.cpp" "src/renderer/Shader.h" "src/renderer/Shader.cpp" "src/renderer/VolumeRenderer.h" "src/renderer/VolumeRenderer.cpp" "src/ui/TransferFunction.h" "src/ui/TransferFunction.cpp" "src/renderer/SmoothPass.h" "src/renderer/SmoothPass.cpp" "src/app/Config.h" "src/app/Config.cpp" "src/app/Utils.h" "src/app/Screenshot.h" "src/app/Screenshot.cpp")
target_include_directories(VolumeRenderer PRIVATE
    src/
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
    C:/GLFW/glfw-3.4.bin.WIN64/include
    C:/imgui/imgui-docking
    C:/imgui/imgui-docking/backends
    C:/GLM/glm-master
    ${CMAKE_SOURCE_DIR}/third_party/stb
)
target_link_libraries(VolumeRenderer PRIVATE
    glfw
    glad
    glm
    imgui
    opengl32
    nfd
)

add_custom_command(TARGET VolumeRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:VolumeRenderer>/shaders
)