/*
* File: CameraSystems.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Scene/CameraSystems.hpp"

#include "Lutum/Core/Math.hpp"
#include "Lutum/Core/Time.hpp"
#include "Lutum/ECS/CommandBuffer.hpp"
#include "Lutum/ECS/Query.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Scheduler.hpp"
#include "Lutum/Platform/Input.hpp"
#include "Lutum/Platform/Window.hpp"
#include "Lutum/Scene/Camera.hpp"
#include "Lutum/Scene/Transform.hpp"

namespace Lutum {

// Just under +/-90 degrees
static constexpr float kPitchLimit = 1.55334303f;

void RegisterCameraSystems(Curia::Scheduler& scheduler) {
    using namespace Curia;

    scheduler.AddSystem("FlyCam")
        .ReadsResource<Input>()
        .ReadsResource<Time>()
        .Writes<Transform>()
        .Writes<FlyCam>()
        .Execute([query = Query<Transform, FlyCam>{}](Registry& registry, CommandBuffer&) mutable {
            const Input& input = registry.GetResource<Input>();
            const float dt = registry.GetResource<Time>().DeltaSeconds();

            query.Refresh(registry);
            query.Each([&](Transform& transform, FlyCam& fly) {
                // Look
                if (input.IsRelativeMouseMode()) {
                    const Vec2 delta = input.MouseDelta();
                    fly.yaw -= delta.x * fly.lookSensitivity;
                    fly.pitch = glm::clamp(fly.pitch - delta.y * fly.lookSensitivity,
                                           -kPitchLimit, kPitchLimit);
                }

                transform.rotation =
                    glm::angleAxis(fly.yaw, Vec3(0.0f, 1.0f, 0.0f)) *
                    glm::angleAxis(fly.pitch, Vec3(1.0f, 0.0f, 0.0f));

                // Move
                Vec3 move(0.0f);
                if (input.IsKeyDown(Key::W)) move += CameraMath::Forward(transform);
                if (input.IsKeyDown(Key::S)) move -= CameraMath::Forward(transform);
                if (input.IsKeyDown(Key::D)) move += CameraMath::Right(transform);
                if (input.IsKeyDown(Key::A)) move -= CameraMath::Right(transform);
                if (input.IsKeyDown(Key::E) || input.IsKeyDown(Key::SPACE)) move += Vec3(0.0f, 1.0f, 0.0f);
                if (input.IsKeyDown(Key::Q) || input.IsKeyDown(Key::LCTRL)) move -= Vec3(0.0f, 1.0f, 0.0f);

                if (glm::dot(move, move) > 0.0f) {
                    float speed = fly.moveSpeed;
                    if (input.IsKeyDown(Key::LSHIFT))
                        speed *= fly.sprintMultiplier;

                    transform.position += glm::normalize(move) * speed * dt;
                }
            });
        });

    scheduler.AddSystem("CameraAspect")
        .ReadsResource<WindowInfo>()
        .Writes<CameraComponent>()
        .Execute([query = Query<CameraComponent>{}](Registry& registry, CommandBuffer&) mutable {
            const WindowInfo& window = registry.GetResource<WindowInfo>();
            if (window.drawableWidth == 0 || window.drawableHeight == 0)
                return;

            const float aspect = static_cast<float>(window.drawableWidth) /
                                 static_cast<float>(window.drawableHeight);

            query.Refresh(registry);
            query.Each([aspect](CameraComponent& camera) {
                camera.aspect = aspect;
            });
        });
}
} // Lutum