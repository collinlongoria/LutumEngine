/*
* File: FlyCameraController.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/FlyCameraController.hpp"

#include "Lutum/Graphics/Camera.hpp"
#include "Lutum/Platform/Input.hpp"

namespace Lutum {
void FlyCameraController::Update(Camera &camera, const Input &input, float deltaSeconds) {
    // Look (only while cursor is captured)
    if (input.IsRelativeMouseMode()) {
        const Vec2 delta = input.MouseDelta();
        camera.AddYawPitch(delta.x * m_lookSensitivity, -delta.y * m_lookSensitivity);
    }

    // Move
    Vec3 move(0.0f);
    if (input.IsKeyDown(Key::W)) move += camera.Forward();
    if (input.IsKeyDown(Key::S)) move -= camera.Forward();
    if (input.IsKeyDown(Key::D)) move += camera.Right();
    if (input.IsKeyDown(Key::A)) move -= camera.Right();
    if (input.IsKeyDown(Key::E) || input.IsKeyDown(Key::SPACE)) move += Vec3(0.0f, 1.0f, 0.0f);
    if (input.IsKeyDown(Key::Q) || input.IsKeyDown(Key::LCTRL)) move -= Vec3(0.0f, 1.0f, 0.0f);

    if (glm::dot(move, move) > 0.0f) {
        float speed = m_moveSpeed;
        if (input.IsKeyDown(Key::LSHIFT))
            speed *= m_sprintMultiplier;

        camera.SetPosition(camera.Position() + glm::normalize(move) * speed * deltaSeconds);
    }
}
} // Lutum