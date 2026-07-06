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
#include <array>
#include <cstddef>

#include "Lutum/Core/Math.hpp"
#include "Lutum/Scene/Transform.hpp"
#include "Lutum/ECS/Reflect.hpp"

namespace Lutum {

struct CameraComponent {
    static constexpr const char* kCuriaName = "CameraComponent";

    float fovY = 1.0471975512f; // 60 degrees
    float aspect = 16.0f / 9.0f; // overwritten each frame from WindowInfo
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    static constexpr auto CuriaFields() {
        using namespace Curia;
        return std::array{
            FieldInfo{"fovY",      offsetof(CameraComponent, fovY),      FieldType::F32, 1},
            FieldInfo{"aspect",    offsetof(CameraComponent, aspect),    FieldType::F32, 1},
            FieldInfo{"nearPlane", offsetof(CameraComponent, nearPlane), FieldType::F32, 1},
            FieldInfo{"farPlane",  offsetof(CameraComponent, farPlane),  FieldType::F32, 1},
        };
    }
};

// Tag: marks the camera the renderer uses
struct ActiveCamera {
    static constexpr const char* kCuriaName = "ActiveCamera";
};

// Fly controller state + tuning
struct FlyCam {
    static constexpr const char* kCuriaName = "FlyCam";

    float yaw = 0.0f; // radians; 0 faces -Z
    float pitch = 0.0f;
    float moveSpeed = 5.0f;
    float sprintMultiplier = 4.0f;
    float lookSensitivity = 0.0025f;

    static constexpr auto CuriaFields() {
        using namespace Curia;
        return std::array{
            FieldInfo{"yaw", offsetof(FlyCam, yaw), FieldType::F32, 1},
            FieldInfo{"pitch", offsetof(FlyCam, pitch), FieldType::F32, 1},
            FieldInfo{"moveSpeed", offsetof(FlyCam, moveSpeed), FieldType::F32, 1},
            FieldInfo{"sprintMultiplier",offsetof(FlyCam, sprintMultiplier), FieldType::F32, 1},
            FieldInfo{"lookSensitivity", offsetof(FlyCam, lookSensitivity), FieldType::F32, 1},
        };
    }
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
