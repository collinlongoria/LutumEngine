/*
* File: BufferUploader.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_BUFFERUPLOADER_HPP
#define LUTUM_BUFFERUPLOADER_HPP
#include <cstdint>
#include <vector>

struct SDL_GPUCommandBuffer;
struct SDL_GPUCopyPass;
struct SDL_GPUTransferBuffer;

namespace Lutum {

class GraphicsDevice;
class Buffer;

/*
 * Batches CPU -> GPU buffer uploads into a single command buffer
 *
 * Usage:
 *  BufferUploader uploader(device);
 *  uploader.Begin();
 *  uploader.Upload(vertexBuffer, vertices.data(), vertexBytes);
 *  uploader.Upload(indexBUffer, indices.data(), indexBytes);
 *  uploader.End(); <- submits, transfer buffers released
 */
class BufferUploader {
public:
    explicit BufferUploader(GraphicsDevice& device);
    ~BufferUploader();

    BufferUploader(const BufferUploader&) = delete;
    BufferUploader& operator=(const BufferUploader&) = delete;

    bool Begin();
    bool Upload(Buffer& dst, const void* data, uint32_t size, uint32_t dstOffset = 0);
    bool End();

private:
    void ReleaseTransferBuffers();

    GraphicsDevice* m_device = nullptr;

    SDL_GPUCommandBuffer* m_cmd = nullptr;
    SDL_GPUCopyPass* m_copyPass = nullptr;

    std::vector<SDL_GPUTransferBuffer*> m_pendingTransfers;
};
} // Lutum

#endif //LUTUM_BUFFERUPLOADER_HPP
