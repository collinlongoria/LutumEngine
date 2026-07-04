/*
* File: DebugUI.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Debug/DebugUI.hpp"

#include "Lutum/Core/Log.hpp"

#if LUTUM_ENABLE_DEBUG_UI

#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>

#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Platform/Window.hpp"

namespace Lutum::Debug::UI {
namespace {
    bool s_initialized = false;
    SDL_GPUDevice* s_device = nullptr;
    bool s_frameOpen = false;
}

bool Initialize(GraphicsDevice& device) {
    if (s_initialized) {
        LUTUM_WARN("DebugUI::Initialize called twice");
        return true;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForSDLGPU(device.GetWindow().NativeHandle())) {
        LUTUM_ERROR("ImGui SDL3 backend init failed");
        ImGui::DestroyContext();
        return false;
    }

    ImGui_ImplSDLGPU3_InitInfo initInfo = {};
    initInfo.Device = device.NativeHandle();
    initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(
        device.NativeHandle(), device.GetWindow().NativeHandle());
    initInfo.MSAASamples = SDL_GPU_SAMPLECOUNT_1;

    if (!ImGui_ImplSDLGPU3_Init(&initInfo)) {
        LUTUM_ERROR("ImGui SDL_GPU backend init failed");
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    s_device = device.NativeHandle();
    s_initialized = true;
    LUTUM_INFO("DebugUI initialized (ImGui {}, docking)", IMGUI_VERSION);
    return true;
}

void Shutdown() {
    if (!s_initialized)
        return;

    SDL_WaitForGPUIdle(s_device);

    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    s_device = nullptr;
    s_initialized = false;
}

bool IsInitialized() { return s_initialized; }

void ProcessEvent(const SDL_Event& event) {
    if (!s_initialized)
        return;
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void BeginFrame() {
    if (!s_initialized)
        return;

    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    s_frameOpen = true;
}

void PrepareRender(SDL_GPUCommandBuffer* cmd) {
    if (!s_initialized || !s_frameOpen)
        return;

    ImGui::Render();
    ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), cmd);
}

void Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass) {
    if (!s_initialized || !s_frameOpen)
        return;

    ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), cmd, pass);
    s_frameOpen = false;
}

} // Lutum::DebugUI

#else // LUTUM_ENABLE_DEBUG_UI off: everything no-ops

namespace Lutum::DebugUI {
bool Initialize(GraphicsDevice&) { return true; }
void Shutdown() {}
bool IsInitialized() { return false; }
void ProcessEvent(const SDL_Event&) {}
void BeginFrame() {}
void PrepareRender(SDL_GPUCommandBuffer*) {}
void Render(SDL_GPUCommandBuffer*, SDL_GPURenderPass*) {}
} // Lutum::DebugUI

#endif