/*
* File: Texture.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/Texture.hpp"

#include <algorithm>
#include <bit>

#include <SDL3/SDL_gpu.h>

#include "Lutum/Core/Log.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"

namespace Lutum {

Texture::Texture(GraphicsDevice& device, SDL_GPUTexture* texture, const CreateInfo& info)
    : m_device(&device), m_texture(texture),
      m_width(info.width), m_height(info.height),
      m_layers(info.layers), m_mipLevels(info.mipLevels)
{
}

Texture::Texture(Texture&& other) noexcept
    : m_device(other.m_device), m_texture(other.m_texture),
      m_width(other.m_width), m_height(other.m_height),
      m_layers(other.m_layers), m_mipLevels(other.m_mipLevels)
{
    other.m_device = nullptr;
    other.m_texture = nullptr;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Release();

        m_device = other.m_device;
        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_layers = other.m_layers;
        m_mipLevels = other.m_mipLevels;

        other.m_device = nullptr;
        other.m_texture = nullptr;
    }
    return *this;
}

std::optional<Texture> Texture::Create(GraphicsDevice& device, const CreateInfo& info, const char* debugName) {
    if (info.width == 0 || info.height == 0 || info.layers == 0) {
        LUTUM_ERROR("Texture::Create with zero dimension");
        return std::nullopt;
    }

    CreateInfo resolved = info;
    if (resolved.mipLevels == 0) {
        const uint32_t largest = std::max(info.width, info.height);
        resolved.mipLevels = std::bit_width(largest);
    }

    SDL_GPUTextureCreateInfo createInfo = {};
    createInfo.type = (info.layers > 1) ? SDL_GPU_TEXTURETYPE_2D_ARRAY : SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    // COLOR_TARGET required by SDL_GenerateMipmapsForGPUTexture
    createInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | (resolved.mipLevels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0);
    createInfo.width = info.width;
    createInfo.height = info.height;
    createInfo.layer_count_or_depth = info.layers;
    createInfo.num_levels = resolved.mipLevels;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device.NativeHandle(), &createInfo);
    if (!texture) {
        LUTUM_ERROR("SDL_CreateGPUTexture failed: {}", SDL_GetError());
        return std::nullopt;
    }

    if (debugName) {
        SDL_SetGPUTextureName(device.NativeHandle(), texture, debugName);
    }

    return Texture{device, texture, resolved};
}

void Texture::Release() {
    if (m_texture) {
        SDL_ReleaseGPUTexture(m_device->NativeHandle(), m_texture);
        m_texture = nullptr;
    }
}

Texture::~Texture() {
    Release();
}
} // Lutum
