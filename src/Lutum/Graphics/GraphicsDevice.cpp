/*
* File: GraphicsDevice.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/GraphicsDevice.hpp"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_gpu.h>

#include "Lutum/Platform/Window.hpp"

namespace Lutum {
GraphicsDevice::GraphicsDevice(Window &window)
    : m_window(&window)
{
}

bool GraphicsDevice::Initialize() {
    m_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true,
        nullptr
    );
    if (!m_device) {
        SDL_Log("SDL_CreateGPUDevice failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(m_device, m_window->NativeHandle())) {
        SDL_Log("SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
        SDL_DestroyGPUDevice(m_device);
        m_device = nullptr;
        return false;
    }

    SDL_Log("GPU Driver: %s", SDL_GetGPUDeviceDriver(m_device));

    m_windowClaimed = true;
    return true;
}

void GraphicsDevice::Shutdown() {
    if (m_windowClaimed) {
        SDL_ReleaseWindowFromGPUDevice(m_device, m_window->NativeHandle());
        m_windowClaimed = false;
    }

    if (m_device) {
        SDL_DestroyGPUDevice(m_device);
        m_device = nullptr;
    }
}

GraphicsDevice::~GraphicsDevice() {
    Shutdown();
}
} // Lutum