#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    Camera(float radius, glm::vec3 target, float fovDegrees);

    void onMouseDrag(float deltaX, float deltaY);
    void onScroll(float delta);
    void onWindowResize(int width, int height);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::vec3 getPosition() const;

    void setView(glm::vec3 direction, glm::vec3 up);

    // for the shader — inverse matrices needed for ray generation
    glm::mat4 getInverseViewMatrix() const;
    glm::mat4 getInverseProjectionMatrix() const;

private:
    void recalculatePosition();

    glm::vec3 m_target;
    glm::vec3 m_position;
    glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_radius;
    float m_theta; // horizontal angle in radians
    float m_phi; // vertical angle in radians

    float m_fovY;
    float m_aspect;
    float m_near = 0.01f;
    float m_far = 1000.0f;

    float m_orbitSensitivity = 0.005f;
    float m_zoomSensitivity = 0.1f;
};