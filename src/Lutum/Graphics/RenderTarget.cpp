/*
* File: RenderTarget.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/RenderTarget.hpp"

#include <SDL3/SDL_gpu.h>

#include "Lutum/Core/Log.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"
#include "Lutum/Platform/Window.hpp"

namespace Lutum {

static SDL_GPUTextureFormat ResolveDepthFormat(SDL_GPUDevice* device) {
    if (SDL_GPUTextureSupportsFormat(device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    }
    return SDL_GPU_TEXTUREFORMAT_D24_UNORM;
}

RenderTarget::RenderTarget(RenderTarget&& other) noexcept
    : m_device(other.m_device),
      m_width(other.m_width), m_height(other.m_height),
      m_offscreen(other.m_offscreen), m_wantDepth(other.m_wantDepth),
      m_colorTexture(other.m_colorTexture), m_depthTexture(other.m_depthTexture),
      m_nativeColorFormat(other.m_nativeColorFormat),
      m_nativeDepthFormat(other.m_nativeDepthFormat)
{
    other.m_device = nullptr;
    other.m_colorTexture = nullptr;
    other.m_depthTexture = nullptr;
}

RenderTarget& RenderTarget::operator=(RenderTarget&& other) noexcept {
    if (this != &other) {
        Release();

        m_device = other.m_device;
        m_width = other.m_width;
        m_height = other.m_height;
        m_offscreen = other.m_offscreen;
        m_wantDepth = other.m_wantDepth;
        m_colorTexture = other.m_colorTexture;
        m_depthTexture = other.m_depthTexture;
        m_nativeColorFormat = other.m_nativeColorFormat;
        m_nativeDepthFormat = other.m_nativeDepthFormat;

        other.m_device = nullptr;
        other.m_colorTexture = nullptr;
        other.m_depthTexture = nullptr;
    }
    return *this;
}

std::optional<RenderTarget> RenderTarget::Create(GraphicsDevice& device, const CreateInfo& info) {
    if (info.width == 0 || info.height == 0) {
        LUTUM_ERROR("RenderTarget::Create with zero size ({}x{})", info.width, info.height);
        return std::nullopt;
    }
    if (info.offscreen && info.colorFormat == TargetFormat::SWAPCHAIN) {
        LUTUM_ERROR("offscreen RenderTarget requires an explicit color format");
        return std::nullopt;
    }

    RenderTarget target;
    target.m_device = &device;
    target.m_width = info.width;
    target.m_height = info.height;
    target.m_offscreen = info.offscreen;
    target.m_wantDepth = info.hasDepth;

    if (info.offscreen) {
        // Only RGBA8_UNORM for now
        target.m_nativeColorFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    } else {
        target.m_nativeColorFormat = SDL_GetGPUSwapchainTextureFormat(
            device.NativeHandle(), device.GetWindow().NativeHandle());
    }

    if (info.hasDepth) {
        target.m_nativeDepthFormat = ResolveDepthFormat(device.NativeHandle());
    }

    if (!target.CreateTextures()) {
        return std::nullopt;
    }

    return target;
}

bool RenderTarget::CreateTextures() {
    SDL_GPUDevice* device = m_device->NativeHandle();

    if (m_offscreen) {
        SDL_GPUTextureCreateInfo colorInfo = {};
        colorInfo.type = SDL_GPU_TEXTURETYPE_2D;
        colorInfo.format = static_cast<SDL_GPUTextureFormat>(m_nativeColorFormat);
        colorInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        colorInfo.width = m_width;
        colorInfo.height = m_height;
        colorInfo.layer_count_or_depth = 1;
        colorInfo.num_levels = 1;

        m_colorTexture = SDL_CreateGPUTexture(device, &colorInfo);
        if (!m_colorTexture) {
            LUTUM_ERROR("RenderTarget color texture creation failed: {}", SDL_GetError());
            return false;
        }
        SDL_SetGPUTextureName(device, m_colorTexture, "render_target_color");
    }

    if (m_wantDepth) {
        SDL_GPUTextureCreateInfo depthInfo = {};
        depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
        depthInfo.format = static_cast<SDL_GPUTextureFormat>(m_nativeDepthFormat);
        depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depthInfo.width = m_width;
        depthInfo.height = m_height;
        depthInfo.layer_count_or_depth = 1;
        depthInfo.num_levels = 1;

        m_depthTexture = SDL_CreateGPUTexture(device, &depthInfo);
        if (!m_depthTexture) {
            LUTUM_ERROR("RenderTarget depth texture creation failed: {}", SDL_GetError());
            Release();
            return false;
        }
        SDL_SetGPUTextureName(device, m_depthTexture, "render_target_depth");
    }

    return true;
}

bool RenderTarget::Resize(uint32_t width, uint32_t height) {
    if (width == m_width && height == m_height)
        return true;
    if (width == 0 || height == 0)
        return false; // minimized; keep old textures, caller skips rendering

    if (m_colorTexture) {
        SDL_ReleaseGPUTexture(m_device->NativeHandle(), m_colorTexture);
        m_colorTexture = nullptr;
    }
    if (m_depthTexture) {
        SDL_ReleaseGPUTexture(m_device->NativeHandle(), m_depthTexture);
        m_depthTexture = nullptr;
    }

    m_width = width;
    m_height = height;

    LUTUM_DEBUG("RenderTarget resized to {}x{}", width, height);
    return CreateTextures();
}

SDL_GPURenderPass* RenderTarget::BeginRenderPass(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchainTexture, const Vec4& clearColor, float clearDepth) {
    SDL_GPUTexture* color = m_offscreen ? m_colorTexture : swapchainTexture;

    LUTUM_ASSERT(m_offscreen == (swapchainTexture == nullptr), "swapchain texture required iff target is swapchain-backed");
    if (!color)
        return nullptr;

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = color;
    colorTarget.clear_color = SDL_FColor{clearColor.r, clearColor.g, clearColor.b, clearColor.a};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    if (m_depthTexture) {
        SDL_GPUDepthStencilTargetInfo depthTarget = {};
        depthTarget.texture = m_depthTexture;
        depthTarget.clear_depth = clearDepth;
        depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

        return SDL_BeginGPURenderPass(cmd, &colorTarget, 1, &depthTarget);
    }

    return SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
}

void RenderTarget::Release() {
    if (!m_device)
        return;

    if (m_colorTexture) {
        SDL_ReleaseGPUTexture(m_device->NativeHandle(), m_colorTexture);
        m_colorTexture = nullptr;
    }
    if (m_depthTexture) {
        SDL_ReleaseGPUTexture(m_device->NativeHandle(), m_depthTexture);
        m_depthTexture = nullptr;
    }
    m_device = nullptr;
}

RenderTarget::~RenderTarget() {
    Release();
}

} // Lutum
