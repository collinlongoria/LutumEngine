/*
* File: Camera.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/Camera.hpp"

namespace Lutum {

// Just under +/-90 degrees
static constexpr float kPitchLimit = 1.55334303f;

void Camera::SetPerspective(float fovYRadians, float aspect, float nearPlane, float farPlane) {
    m_fovY = fovYRadians;
    m_aspect = aspect;
    m_near = nearPlane;
    m_far = farPlane;
}

void Camera::SetAspect(float aspect) {
    if (aspect > 0.0f)
        m_aspect = aspect;
}

void Camera::SetYawPitch(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = glm::clamp(pitch, -kPitchLimit, kPitchLimit);
}

void Camera::AddYawPitch(float deltaYaw, float deltaPitch) {
    SetYawPitch(m_yaw + deltaYaw, m_pitch + deltaPitch);
}

Vec3 Camera::Forward() const {
    return glm::normalize(Vec3(
        glm::cos(m_pitch) * glm::cos(m_yaw),
        glm::sin(m_pitch),
        glm::cos(m_pitch) * glm::sin(m_yaw)
    ));
}

Vec3 Camera::Right() const {
    return glm::normalize(glm::cross(Forward(), Vec3(0.0f, 1.0f, 0.0f)));
}

Vec3 Camera::Up() const {
    return glm::normalize(glm::cross(Right(), Forward()));
}

Mat4 Camera::ViewMatrix() const {
    return glm::lookAt(m_position, m_position + Forward(), Vec3(0.0f, 1.0f, 0.0f));
}

Mat4 Camera::ProjectionMatrix() const {
    // GLM_FORCE_DEPTH_ZERO_TO_ONE gives [0,1] depth, matching SDL_GPU.
    return glm::perspective(m_fovY, m_aspect, m_near, m_far);
}

Mat4 Camera::ViewProjectionMatrix() const {
    return ProjectionMatrix() * ViewMatrix();
}
} // Lutum