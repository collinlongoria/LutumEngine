/*
* File: GpuUploader.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/GpuUploader.hpp"

#include <cstring>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>

#include "Texture.hpp"
#include "Lutum/Core/Log.hpp"
#include "Lutum/Graphics/Buffer.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"

namespace Lutum {
GpuUploader::GpuUploader(GraphicsDevice &device)
    : m_device(&device)
{
}

bool GpuUploader::Begin() {
    if (m_cmd) {
        SDL_Log("GpuUploader::Begin called while a batch is already open");
        return false;
    }

    m_cmd = SDL_AcquireGPUCommandBuffer(m_device->NativeHandle());
    if (!m_cmd) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return false;
    }

    m_copyPass = SDL_BeginGPUCopyPass(m_cmd);
    if (!m_copyPass) {
        SDL_Log("SDL_BeginGPUCopyPass failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(m_cmd);
        m_cmd = nullptr;
        return false;
    }

    return true;
}

bool GpuUploader::Upload(Buffer &dst, const void *data, uint32_t size, uint32_t dstOffset) {
    if (!m_copyPass) {
        SDL_Log("GpuUploader::Upload called outside Begin/End");
        return false;
    }

    if (!dst.IsValid() || !data || size == 0)
        return false;

    if (dstOffset + size > dst.Size()) {
        SDL_Log("GpuUploader::Upload out of range (offset %u + size %u > buffer size %u)",
                dstOffset, size, dst.Size());
        return false;
    }

    // TODO: could pool these transfers. currently one per upload
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = size;
    transferInfo.props = 0;

    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(m_device->NativeHandle(), &transferInfo);
    if (!transfer) {
        SDL_Log("SDL_CreateGPUTransferBuffer failed: %s", SDL_GetError());
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(m_device->NativeHandle(), transfer, false);
    if (!mapped) {
        SDL_Log("SDL_MapGPUTransferBuffer failed: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(m_device->NativeHandle(), transfer);
        return false;
    }

    std::memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(m_device->NativeHandle(), transfer);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion region = {};
    region.buffer = dst.NativeHandle();
    region.offset = dstOffset;
    region.size = size;

    SDL_UploadToGPUBuffer(m_copyPass, &src, &region, false);

    m_pendingTransfers.push_back(transfer);
    return true;
}

bool GpuUploader::Upload(Texture& dst, const void* pixels, uint32_t byteSize, uint32_t layer) {
    if (!m_copyPass) {
        LUTUM_ERROR("GpuUploader::Upload(texture) called outside Begin/End");
        return false;
    }
    if (!dst.IsValid() || !pixels || byteSize == 0 || layer >= dst.Layers())
        return false;

    const uint32_t expected = dst.Width() * dst.Height() * 4;
    if (byteSize != expected) {
        LUTUM_ERROR("texture upload size mismatch: got {}, expected {} ({}x{} RGBA8)",
                    byteSize, expected, dst.Width(), dst.Height());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = byteSize;

    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(m_device->NativeHandle(), &transferInfo);
    if (!transfer) {
        LUTUM_ERROR("SDL_CreateGPUTransferBuffer failed: {}", SDL_GetError());
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(m_device->NativeHandle(), transfer, false);
    if (!mapped) {
        LUTUM_ERROR("SDL_MapGPUTransferBuffer failed: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(m_device->NativeHandle(), transfer);
        return false;
    }
    std::memcpy(mapped, pixels, byteSize);
    SDL_UnmapGPUTransferBuffer(m_device->NativeHandle(), transfer);

    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;   // rows tightly packed

    SDL_GPUTextureRegion region = {};
    region.texture = dst.NativeHandle();
    region.mip_level = 0;
    region.layer = layer;
    region.w = dst.Width();
    region.h = dst.Height();
    region.d = 1;

    SDL_UploadToGPUTexture(m_copyPass, &src, &region, false);
    m_pendingTransfers.push_back(transfer);

    if (dst.MipLevels() > 1) {
        m_pendingMipmaps.push_back(dst.NativeHandle());
    }
    return true;
}

bool GpuUploader::End() {
    if (!m_cmd) {
        SDL_Log("GpuUploader::End called without Begin");
        return false;
    }

    SDL_EndGPUCopyPass(m_copyPass);
    m_copyPass = nullptr;

    for (SDL_GPUTexture* texture : m_pendingMipmaps) {
        SDL_GenerateMipmapsForGPUTexture(m_cmd, texture);
    }
    m_pendingMipmaps.clear();

    const bool submitted = SDL_SubmitGPUCommandBuffer(m_cmd);
    if (!submitted) {
        SDL_Log("SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
    }
    m_cmd = nullptr;
    
    ReleaseTransferBuffers();

    return submitted;
}

void GpuUploader::ReleaseTransferBuffers() {
    for (SDL_GPUTransferBuffer* transfer : m_pendingTransfers) {
        SDL_ReleaseGPUTransferBuffer(m_device->NativeHandle(), transfer);
    }
    m_pendingTransfers.clear();
}

GpuUploader::~GpuUploader() {
    if (m_cmd) {
        LUTUM_WARN("GpuUploader: destroyed with an open batch; cancelling");
        if (m_copyPass) {
            SDL_EndGPUCopyPass(m_copyPass);
        }
        SDL_CancelGPUCommandBuffer(m_cmd);
        ReleaseTransferBuffers();
        m_pendingMipmaps.clear();
    }
}
} // Lutum