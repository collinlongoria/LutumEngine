/*
* File: Camera.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CAMERA_HPP
#define LUTUM_CAMERA_HPP
#include "Lutum/Core/Math.hpp"

namespace Lutum {
class Camera {
public:
    Camera() = default;

    void SetPerspective(float fovYRadians, float aspect, float nearPlane, float farPlane);
    void SetAspect(float aspect);

    void SetPosition(const Vec3& position) { m_position = position; }
    [[nodiscard]]
    const Vec3& Position() const { return m_position; }

    // Yaw/pitch in radians. Pitch is clamped to avoid gimbal flip.
    void SetYawPitch(float yaw, float pitch);
    void AddYawPitch(float deltaYaw, float deltaPitch);

    [[nodiscard]]
    float Yaw() const { return m_yaw; }
    [[nodiscard]]
    float Pitch() const { return m_pitch; }

    [[nodiscard]]
    Vec3 Forward() const;
    [[nodiscard]]
    Vec3 Right() const;
    [[nodiscard]]
    Vec3 Up() const;

    [[nodiscard]]
    Mat4 ViewMatrix() const;
    [[nodiscard]]
    Mat4 ProjectionMatrix() const;
    [[nodiscard]]
    Mat4 ViewProjectionMatrix() const;

private:
    Vec3 m_position = Vec3(0.0f);

    // Yaw of -pi/2 faces -Z, matching the view convention.
    float m_yaw = -1.57079632679f;
    float m_pitch = 0.0f;

    float m_fovY = 1.0471975512f; // 60 degrees
    float m_aspect = 16.0f / 9.0f;
    float m_near = 0.1f;
    float m_far = 1000.0f;
};
} // Lutum

#endif //LUTUM_CAMERA_HPP
