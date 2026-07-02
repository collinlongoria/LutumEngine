/*
* File: Shader.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/Shader.hpp"

#include <SDL3/SDL_gpu.h>

#include "Lutum/Graphics/GraphicsDevice.hpp"

namespace Lutum {
Shader::Shader(GraphicsDevice &device, SDL_GPUShader *shader)
    : m_device(&device), m_shader(shader)
{
}

Shader::Shader(Shader &&other) noexcept
    : m_device(other.m_device), m_shader(other.m_shader)
{
    other.m_device = nullptr;
    other.m_shader = nullptr;
}

Shader &Shader::operator=(Shader &&other) noexcept {
    if (this != &other) {
        Release();

        m_device = other.m_device;
        m_shader = other.m_shader;

        other.m_device = nullptr;
        other.m_shader = nullptr;
    }

    return *this;
}

void Shader::Release() {
    if (m_shader) {
        SDL_ReleaseGPUShader(m_device->NativeHandle(), m_shader);
        m_shader = nullptr;
    }
}

Shader::~Shader() {
    Release();
}
} // Lutum