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

#include <imgui.h>

#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Jobs.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/Core/Time.hpp"
#include "Lutum/Debug/DebugUI.hpp"
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

bool Application::Initialize(const char* projectPath) {
    Log::Initialize();
    Jobs::Initialize();

    if (!FileSystem::Initialize())
        return false;
    if (!FileSystem::MountProject(projectPath))
        return false;

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

    RenderTarget::CreateInfo targetInfo = {};
    targetInfo.width = 1280;
    targetInfo.height = 720;
    targetInfo.offscreen = true;
    targetInfo.colorFormat = TargetFormat::RGBA8_UNORM;
    targetInfo.hasDepth = true;

    std::optional<RenderTarget> target = RenderTarget::Create(*m_graphicsDevice, targetInfo);
    if (!target)
        return false;
    m_sceneTarget = std::move(*target);

    m_renderer = std::make_unique<Renderer>(*m_graphicsDevice);
    if (!m_renderer->Initialize(m_sceneTarget))
        return false;

    if (!Debug::UI::Initialize(*m_graphicsDevice))
        return false;

    const bool firstRun = !FileSystem::Exists("/Saved/imgui.ini");
    if (FileSystem::EnsureDirectory("/Saved/")) {
        m_imguiIniPath = FileSystem::Resolve("/Saved/imgui.ini");
        if (!m_imguiIniPath.empty())
            ImGui::GetIO().IniFilename = m_imguiIniPath.c_str();
    }
    if (firstRun)
        m_editorUI.RequestLayoutReset();

    m_window->SetEventHook([](const SDL_Event& event) {
        Debug::UI::ProcessEvent(event);
    });

    // Resources
    m_registry.SetResource<Time>();
    m_registry.SetResource<Input>();
    m_registry.SetResource<ViewportInfo>();

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

    m_registry.GetResource<Input>().SetRelativeMouseMode(*m_window, false);

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

        // Editor fly controls
        const ViewportInfo& viewport = m_registry.GetResource<ViewportInfo>();
        const bool wantFly = viewport.hovered && input.IsMouseButtonDown(MouseButton::RIGHT);
        if (wantFly != input.IsRelativeMouseMode())
            input.SetRelativeMouseMode(*m_window, wantFly);

        Debug::UI::BeginFrame();
        m_editorUI.Draw(m_registry, m_scheduler, m_sceneTarget); // writes ViewportInfo, resizes target

        if (m_editorUI.Context().exitRequested)
            m_window->RequestClose();

        m_scheduler.Run();

        if (m_renderer->BeginFrame()) {
            m_renderer->RenderScene(m_registry, m_sceneTarget);
            m_renderer->RenderDebugUI();
            m_renderer->EndFrame();
        }
    }
}

void Application::Shutdown() {
    if (m_shutdown) return;
    m_shutdown = true;

    Debug::UI::Shutdown();

    m_renderer.reset();

    m_sceneTarget = RenderTarget();

    m_graphicsDevice.reset();
    m_window.reset();
    m_platform.reset();

    Jobs::Shutdown();
    FileSystem::Shutdown();
    Log::Shutdown();
}
} // Lutum