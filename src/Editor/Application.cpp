/*
* File: Application.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Editor/Application.hpp"

#include <chrono>
#include <thread>

#include "Lutum/Core/Jobs.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/Core/Time.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Graphics/Renderer.hpp"
#include "Lutum/Platform/Input.hpp"
#include "Lutum/Platform/PlatformContext.hpp"
#include "Lutum/Platform/Window.hpp"
#include "Lutum/Scene/Camera.hpp"
#include "Lutum/Scene/CameraSystems.hpp"
#include "Lutum/Scene/Transform.hpp"

namespace Lutum {
Application::Application() = default;
Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    Log::Initialize();
    Jobs::Initialize();

    m_platform = std::make_unique<Lutum::PlatformContext>();
    if (!m_platform->IsValid())
        return false;

    m_window = std::make_unique<Lutum::Window>("Lutum", 1280, 720);
    if (!m_window)
        return false;

    m_graphicsDevice = std::make_unique<Lutum::GraphicsDevice>(*m_window);
    if (!m_graphicsDevice)
        return false;
    if (!m_graphicsDevice->Initialize())
        return false;

    m_renderer = std::make_unique<Lutum::Renderer>(*m_graphicsDevice);
    if (!m_renderer->Initialize())
        return false;

    // Resources
    m_registry.SetResource<Time>();
    m_registry.SetResource<Input>();
    m_registry.SetResource<WindowInfo>();

    // Systems
    RegisterCameraSystems(m_scheduler);
    m_scheduler.Compile();
    m_scheduler.LogGraph();

    // Camera entity
    Curia::Entity camera = m_registry.Create();
    m_registry.Add(camera, Transform{Vec3(0.0f, 0.0f, 2.0f)});
    m_registry.Add(camera, CameraComponent{});
    m_registry.Add(camera, FlyCam{});
    m_registry.Add<ActiveCamera>(camera);

    m_registry.GetResource<Input>().SetRelativeMouseMode(*m_window, true);

    return true;
}

void Application::Run() {
    while (!m_window->ShouldClose()) {
        m_registry.GetResource<Time>().Tick();
        m_window->PollEvents();

        if (m_window->IsMinimized()) {
            Debug::SleepMilliseconds(10);
            continue;
        }

        Input& input = m_registry.GetResource<Input>();
        input.Update();

        if (input.WasKeyPressed(Key::ESCAPE))
            input.SetRelativeMouseMode(*m_window, false);
        if (!input.IsRelativeMouseMode() && input.WasMouseButtonPressed(MouseButton::LEFT))
            input.SetRelativeMouseMode(*m_window, true);

        WindowInfo& windowInfo = m_registry.GetResource<WindowInfo>();
        windowInfo.drawableWidth = static_cast<uint32_t>(m_window->DrawableWidth());
        windowInfo.drawableHeight = static_cast<uint32_t>(m_window->DrawableHeight());

        // Simulation
        m_scheduler.Run();

        // Presentation
        m_renderer->RenderFrame(m_registry);
    }
}

void Application::Shutdown() {
    if (m_shutdown) return;
    m_shutdown = true;

    m_renderer.reset();
    m_graphicsDevice.reset();
    m_window.reset();
    m_platform.reset();

    Jobs::Shutdown();
    Log::Shutdown();
}
} // Lutum