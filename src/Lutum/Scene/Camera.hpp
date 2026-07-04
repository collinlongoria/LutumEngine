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
#include "Lutum/Scene/Transform.hpp"

namespace Lutum {

struct CameraComponent {
    float fovY = 1.0471975512f; // 60 degrees
    float aspect = 16.0f / 9.0f; // overwritten each frame from WindowInfo
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
};

// Tag: marks the camera the renderer uses
// Keep it on exactly one entity
struct ActiveCamera {};

// Fly controller state + tuning
struct FlyCam {
    float yaw = 0.0f; // radians; 0 faces -Z
    float pitch = 0.0f;
    float moveSpeed = 5.0f;
    float sprintMultiplier = 4.0f;
    float lookSensitivity = 0.0025f;
};

namespace CameraMath {
    [[nodiscard]]
    inline Vec3 Forward(const Transform& transform) {
        return transform.rotation * Vec3(0.0f, 0.0f, -1.0f);
    }

    [[nodiscard]]
    inline Vec3 Right(const Transform& transform) {
        return transform.rotation * Vec3(1.0f, 0.0f, 0.0f);
    }

    [[nodiscard]]
    inline Mat4 ViewMatrix(const Transform& transform) {
        const Vec3 forward = Forward(transform);
        const Vec3 up = transform.rotation * Vec3(0.0f, 1.0f, 0.0f);
        return glm::lookAt(transform.position, transform.position + forward, up);
    }

    [[nodiscard]]
    inline Mat4 ProjectionMatrix(const CameraComponent& camera) {
        // GLM_FORCE_DEPTH_ZERO_TO_ONE matches SDL_GPU's [0,1] depth.
        return glm::perspective(camera.fovY, camera.aspect, camera.nearPlane, camera.farPlane);
    }

    [[nodiscard]]
    inline Mat4 ViewProjection(const Transform& transform, const CameraComponent& camera) {
        return ProjectionMatrix(camera) * ViewMatrix(transform);
    }
} // CameraMath
} // Lutum

#endif //LUTUM_CAMERA_HPP
