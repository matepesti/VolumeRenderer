#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <algorithm>

Camera::Camera(float radius, glm::vec3 target, float fovDegrees)
    : m_radius(radius)
    , m_target(target)
    , m_fovY(glm::radians(fovDegrees))
    , m_aspect(16.0f / 9.0f)
    , m_theta(glm::radians(45.0f))
    , m_phi(glm::radians(60.0f)) {
    recalculatePosition();
}

void Camera::onMouseDrag(float deltaX, float deltaY) {
    m_up = glm::vec3(0.f, 1.f, 0.f);
    m_theta -= deltaX * m_orbitSensitivity;
    m_phi -= deltaY * m_orbitSensitivity;

    // clamp phi so camera never flips upside down
    // leave a small epsilon away from the poles
    const float epsilon = 0.01f;
    m_phi = std::clamp(m_phi, epsilon, glm::pi<float>() - epsilon);

    recalculatePosition();
}

void Camera::onScroll(float delta) {
    m_radius -= delta * m_zoomSensitivity * m_radius;
    // prevent zooming through or past the target
    m_radius = std::max(m_radius, 0.1f);
    recalculatePosition();
}

void Camera::onWindowResize(int width, int height) {
    if (height > 0)
        m_aspect = static_cast<float>(width) / static_cast<float>(height);
}

void Camera::recalculatePosition() {
    // convert spherical coordinates to cartesian
    m_position.x = m_target.x + m_radius * std::sin(m_phi) * std::sin(m_theta);
    m_position.y = m_target.y + m_radius * std::cos(m_phi);
    m_position.z = m_target.z + m_radius * std::sin(m_phi) * std::cos(m_theta);
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_target, m_up);
}

glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(m_fovY, m_aspect, m_near, m_far);
}

glm::vec3 Camera::getPosition() const {
    return m_position;
}

glm::mat4 Camera::getInverseViewMatrix() const {
    return glm::inverse(getViewMatrix());
}

glm::mat4 Camera::getInverseProjectionMatrix() const {
    return glm::inverse(getProjectionMatrix());
}

void Camera::setView(glm::vec3 direction, glm::vec3 up) {
    m_up = up;
    glm::vec3 offset = glm::normalize(-direction);
    m_phi = std::acos(std::clamp(offset.y,-1.f,1.f));
    m_theta = std::atan2(offset.x, offset.z);
    recalculatePosition();
}