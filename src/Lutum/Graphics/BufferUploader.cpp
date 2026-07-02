/*
* File: BufferUploader.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/BufferUploader.hpp"

#include <cstring>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>

#include "Lutum/Graphics/Buffer.hpp"
#include "Lutum/Graphics/GraphicsDevice.hpp"

namespace Lutum {
BufferUploader::BufferUploader(GraphicsDevice &device)
    : m_device(&device)
{
}

bool BufferUploader::Begin() {
    if (m_cmd) {
        SDL_Log("BufferUploader::Begin called while a batch is already open");
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

bool BufferUploader::Upload(Buffer &dst, const void *data, uint32_t size, uint32_t dstOffset) {
    if (!m_copyPass) {
        SDL_Log("BufferUploader::Upload called outside Begin/End");
        return false;
    }

    if (!dst.IsValid() || !data || size == 0)
        return false;

    if (dstOffset + size > dst.Size()) {
        SDL_Log("BufferUploader::Upload out of range (offset %u + size %u > buffer size %u)",
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

bool BufferUploader::End() {
    if (!m_cmd) {
        SDL_Log("BufferUploader::End called without Begin");
        return false;
    }

    SDL_EndGPUCopyPass(m_copyPass);
    m_copyPass = nullptr;

    const bool submitted = SDL_SubmitGPUCommandBuffer(m_cmd);
    if (!submitted) {
        SDL_Log("SDL_SubmitGPUCommandBuffer failed: %s", SDL_GetError());
    }
    m_cmd = nullptr;
    
    ReleaseTransferBuffers();

    return submitted;
}

void BufferUploader::ReleaseTransferBuffers() {
    for (SDL_GPUTransferBuffer* transfer : m_pendingTransfers) {
        SDL_ReleaseGPUTransferBuffer(m_device->NativeHandle(), transfer);
    }
    m_pendingTransfers.clear();
}

BufferUploader::~BufferUploader() {
    if (m_cmd) {
        SDL_Log("BufferUploader destroyed with an open batch; cancelling");
        if (m_copyPass) {
            SDL_EndGPUCopyPass(m_copyPass);
        }
        SDL_CancelGPUCommandBuffer(m_cmd);
        ReleaseTransferBuffers();
    }
}
} // Lutum