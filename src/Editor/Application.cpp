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
#include <filesystem>

#include <imgui.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_process.h>

#include "Lutum/Assets/AssetRegistry.hpp"
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
#include "Lutum/Scene/Name.hpp"
#include "Lutum/Scene/Transform.hpp"

namespace Lutum {
Application::Application() = default;
Application::~Application() {
    Shutdown();
}

bool Application::Initialize(const char* projectPath, const char* executablePath) {
    m_executablePath = executablePath ? executablePath : "";

    Log::Initialize();
    Jobs::Initialize();

    if (!FileSystem::Initialize())
        return false;
    if (!FileSystem::MountProject(projectPath))
        return false;

    // Asset Systems
    Assets::Initialize();

    m_projectPath = std::filesystem::absolute(projectPath).string();
    auto recent = LoadRecentProjects();
    AddRecentProject(recent, m_projectPath);
    SaveRecentProjects(recent);
    m_editorUI.SetRecentProjects(recent);

    m_settings = EditorSettings::Load();
    m_settings.ApplyTo(m_editorUI.Context());

    m_platform = std::make_unique<Lutum::PlatformContext>();
    if (!m_platform->IsValid())
        return false;

    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable)) {
        m_settings.windowWidth = std::min(m_settings.windowWidth, usable.w);
        m_settings.windowHeight = std::min(m_settings.windowHeight, usable.h);
    }

    m_window = std::make_unique<Lutum::Window>("Lutum",
        m_settings.windowWidth, m_settings.windowHeight);
    if (!m_window)
        return false;
    if (m_settings.windowMaximized)
        m_window->Maximize();

    m_editorUI.SetLayoutWorkSize(m_settings.layoutWorkWidth, m_settings.layoutWorkHeight);

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

    m_window->SetEventHook([this](const SDL_Event& event) {
        Debug::UI::ProcessEvent(event);
        if (event.type == SDL_EVENT_MOUSE_WHEEL)
            m_registry.GetResource<Input>().AddWheelDelta(event.wheel.y);
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
    m_cameraEntity = m_registry.Create();
    m_registry.Add(m_cameraEntity, Transform{Vec3(0.0f, 0.0f, 2.0f)});
    m_registry.Add(m_cameraEntity, CameraComponent{});
    FlyCam fly{};
    fly.moveSpeed = m_settings.cameraMoveSpeed;
    m_registry.Add(m_cameraEntity, fly);
    m_registry.Add<ActiveCamera>(m_cameraEntity);
    Name name = MakeName("Cammy the Camera");
    m_registry.Add(m_cameraEntity, name);

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
        const bool rmbDown = input.IsMouseButtonDown(MouseButton::RIGHT);
        if (!input.IsRelativeMouseMode()) {
            if (viewport.hovered && rmbDown)
                input.SetRelativeMouseMode(*m_window, true);
        }
        else if (!rmbDown) {
            input.SetRelativeMouseMode(*m_window, false);
        }
        ImGuiIO& io = ImGui::GetIO();
        if (input.IsRelativeMouseMode())
            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        else
            io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;

        Debug::UI::BeginFrame();
        m_editorUI.Draw(m_registry, m_scheduler, m_sceneTarget); // writes ViewportInfo, resizes target

        EditorContext& ctx = m_editorUI.Context();
        if (!ctx.relaunchProjectPath.empty()) {
            if (!m_executablePath.empty()) {
                const char* args[] = {m_executablePath.c_str(),
                                      ctx.relaunchProjectPath.c_str(), nullptr};
                if (SDL_Process* child = SDL_CreateProcess(args, false))
                    SDL_DestroyProcess(child); // detach; does not kill the child
                else
                    LUTUM_ERROR("Relaunch failed: {}", SDL_GetError());
            }
            ctx.relaunchProjectPath.clear();
            m_window->RequestClose();
        }
        if (ctx.exitRequested)
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

    if (FileSystem::IsProjectMounted()) {
        m_settings.CaptureFrom(m_editorUI.Context());
        if (const FlyCam* fly = m_registry.Get<FlyCam>(m_cameraEntity))
            m_settings.cameraMoveSpeed = fly->moveSpeed;

        m_settings.layoutWorkWidth = m_editorUI.LayoutWorkWidth();
        m_settings.layoutWorkHeight = m_editorUI.LayoutWorkHeight();
        m_settings.windowMaximized = m_window->IsMaximized();
        if (!m_settings.windowMaximized) {
            m_settings.windowWidth = m_window->Width();
            m_settings.windowHeight = m_window->Height();
        }
        m_settings.Save();
    }

    Debug::UI::Shutdown();

    m_renderer.reset();

    m_sceneTarget = RenderTarget();

    m_graphicsDevice.reset();
    m_window.reset();
    m_platform.reset();

    Jobs::Shutdown();
    Assets::Shutdown();
    FileSystem::Shutdown();
    Log::Shutdown();
}
} // Lutum