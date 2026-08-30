cmake_minimum_required(VERSION 3.24)

project(VolumeRenderer VERSION 1.0 LANGUAGES C CXX)

# C++ configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer static libraries for fetched dependencies.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)

# GLM
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_MakeAvailable(glm)

# Dear ImGui

# We use the docking branch because the application uses:
#     ImGuiConfigFlags_DockingEnable
# and the GLFW/OpenGL3 backends.

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)

FetchContent_MakeAvailable(imgui)


# GLAD
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
)


# Native File Dialog
if(WIN32)

    add_library(nfd STATIC
        ${CMAKE_SOURCE_DIR}/third_party/nfd/nfd_win.cpp
    )

    target_include_directories(nfd PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party/nfd/include
    )

    target_link_libraries(nfd PRIVATE
        comctl32
        ole32
        uuid
    )

else()

    message(FATAL_ERROR
        "Native File Dialog is currently implemented only for Windows."
    )

endif()

# Dear ImGui
add_library(imgui STATIC

    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC

    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends

)

target_link_libraries(imgui PUBLIC
    glfw
)

# VolumeRenderer source files
set(VOLUME_RENDERER_SOURCES

    src/main.cpp

    # Application
    src/app/App.cpp
    src/app/Config.cpp
    src/app/Screenshot.cpp

    # Camera
    src/camera/Camera.cpp

    # Volume
    src/volume/Volume.cpp

    # Renderer
    src/renderer/Shader.cpp
    src/renderer/VolumeRenderer.cpp
    src/renderer/SmoothPass.cpp

    # UI
    src/ui/TransferFunction.cpp
)

# Executable
add_executable(VolumeRenderer
    ${VOLUME_RENDERER_SOURCES}
)

# Include directories
target_include_directories(VolumeRenderer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    # Local dependencies
    ${CMAKE_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_SOURCE_DIR}/third_party/stb
    ${CMAKE_SOURCE_DIR}/third_party/nfd/include
)

# Libraries

target_link_libraries(VolumeRenderer PRIVATE
    glfw
    glm::glm
    glad
    imgui
    nfd
)

if(WIN32)
    target_link_libraries(VolumeRenderer PRIVATE
        opengl32
    )

endif()

# Copy shaders beside executable
add_custom_command(
    TARGET VolumeRenderer
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory

        ${CMAKE_SOURCE_DIR}/shaders

        $<TARGET_FILE_DIR:VolumeRenderer>/shaders
)

# Organize Visual Studio solution
source_group(
    TREE ${CMAKE_SOURCE_DIR}/src
    FILES ${VOLUME_RENDERER_SOURCES}
)