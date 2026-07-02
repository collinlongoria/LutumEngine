/*
* File: Buffer.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/Buffer.hpp"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

#include "Lutum/Graphics/GraphicsDevice.hpp"

namespace Lutum {

static SDL_GPUBufferUsageFlags ToSDLUsage(BufferUsage usage) {
    switch (usage) {
        case BufferUsage::VERTEX:
            return SDL_GPU_BUFFERUSAGE_VERTEX;
        case BufferUsage::INDEX:
            return SDL_GPU_BUFFERUSAGE_INDEX;
        case BufferUsage::INDIRECT:
            return SDL_GPU_BUFFERUSAGE_INDIRECT;
        case BufferUsage::STORAGE_READ:
            return SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    }

    SDL_assert(false);
    return SDL_GPU_BUFFERUSAGE_VERTEX;
}

Buffer::Buffer(GraphicsDevice &device, SDL_GPUBuffer *buffer, uint32_t size)
    : m_device(&device), m_buffer(buffer), m_size(size)
{
}


Buffer::Buffer(Buffer &&other) noexcept
    : m_device(other.m_device), m_buffer(other.m_buffer), m_size(other.m_size)
{
    other.m_device = nullptr;
    other.m_buffer = nullptr;
    other.m_size = 0;
}

Buffer &Buffer::operator=(Buffer &&other) noexcept {
    if (this != &other) {
        Release();

        m_device = other.m_device;
        m_buffer = other.m_buffer;
        m_size = other.m_size;

        other.m_device = nullptr;
        other.m_buffer = nullptr;
        other.m_size = 0;
    }

    return *this;
}

std::optional<Buffer> Buffer::Create(GraphicsDevice &device, BufferUsage usage, uint32_t size, const char *debugName) {
    if (size == 0) {
        SDL_Log("Buffer::Create called with size 0");
        return std::nullopt;
    }

    SDL_GPUBufferCreateInfo createInfo = {};
    createInfo.usage = ToSDLUsage(usage);
    createInfo.size = size;
    createInfo.props = 0;

    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device.NativeHandle(), &createInfo);
    if (!buffer) {
        SDL_Log("SDL_CreateGPUBuffer failed: %s", SDL_GetError());
        return std::nullopt;
    }

    if (debugName) {
        SDL_SetGPUBufferName(device.NativeHandle(), buffer, debugName);
    }

    return Buffer{device, buffer, size};
}

void Buffer::Release() {
    if (m_buffer) {
        SDL_ReleaseGPUBuffer(m_device->NativeHandle(), m_buffer);
        m_buffer = nullptr;
        m_size = 0;
    }
}

Buffer::~Buffer() {
    Release();
}

} // Lutum