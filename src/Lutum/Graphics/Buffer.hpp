/*
* File: Buffer.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_BUFFER_HPP
#define LUTUM_BUFFER_HPP
#include <cstdint>
#include <optional>

struct SDL_GPUBuffer;

namespace Lutum {

class GraphicsDevice;

enum class BufferUsage {
    VERTEX,
    INDEX,
    INDIRECT,
    STORAGE_READ
};

class Buffer {
public:
    Buffer() = default;
    Buffer(GraphicsDevice& device, SDL_GPUBuffer* buffer, uint32_t size);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    static std::optional<Buffer> Create(GraphicsDevice& device, BufferUsage usage, uint32_t size, const char* debugName = nullptr);

    [[nodiscard]]
    bool IsValid() const { return m_buffer != nullptr; }
    [[nodiscard]]
    SDL_GPUBuffer* NativeHandle() const { return m_buffer; }
    [[nodiscard]]
    uint32_t Size() const { return m_size; }

private:
    void Release();

    GraphicsDevice* m_device = nullptr;
    SDL_GPUBuffer* m_buffer = nullptr;
    uint32_t m_size = 0;
};
} // Lutum

#endif //LUTUM_BUFFER_HPP
