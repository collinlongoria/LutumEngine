/*
* File: GpuUploader.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_GPUUPLOADER_HPP
#define LUTUM_GPUUPLOADER_HPP
#include <cstdint>
#include <vector>

struct SDL_GPUCommandBuffer;
struct SDL_GPUCopyPass;
struct SDL_GPUTransferBuffer;
struct SDL_GPUTexture;

namespace Lutum {

class GraphicsDevice;
class Buffer;
class Texture;

/*
 * Batches CPU -> GPU buffer uploads into a single command buffer
 *
 * Usage:
 *  GpuUploader uploader(device);
 *  uploader.Begin();
 *  uploader.Upload(vertexBuffer, vertices.data(), vertexBytes);
 *  uploader.Upload(indexBUffer, indices.data(), indexBytes);
 *  uploader.End(); <- submits, transfer buffers released
 */
class GpuUploader {
public:
    explicit GpuUploader(GraphicsDevice& device);
    ~GpuUploader();

    GpuUploader(const GpuUploader&) = delete;
    GpuUploader& operator=(const GpuUploader&) = delete;

    bool Begin();

    // Buffer upload
    bool Upload(Buffer& dst, const void* data, uint32_t size, uint32_t dstOffset = 0);

    // Uploads tightly-packed RGBA8 pixels to mip 0 of one layer
    // If the texture has >1 mip level, full-chain generation is queued and runs at End() (mipmap gen must happen outside the copy pass)
    bool Upload(Texture& dst, const void* pixels, uint32_t byteSize, uint32_t layer = 0);

    bool End();

private:
    void ReleaseTransferBuffers();

    GraphicsDevice* m_device = nullptr;

    SDL_GPUCommandBuffer* m_cmd = nullptr;
    SDL_GPUCopyPass* m_copyPass = nullptr;

    std::vector<SDL_GPUTransferBuffer*> m_pendingTransfers;
    std::vector<SDL_GPUTexture*> m_pendingMipmaps;
};
} // Lutum

#endif //LUTUM_GPUUPLOADER_HPP
