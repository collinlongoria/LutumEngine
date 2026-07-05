/*
* File: Texture.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_TEXTURE_HPP
#define LUTUM_TEXTURE_HPP
#include <cstdint>
#include <optional>

struct SDL_GPUTexture;

namespace Lutum {

class GraphicsDevice;

class Texture {
public:
    struct CreateInfo {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t layers = 1; // >1 = 2D array
        uint32_t mipLevels = 0; // 0 = full chain
        // TODO: albedo should eventually be SRGB with linear shading
    };

    Texture() = default;
    Texture(GraphicsDevice& device, SDL_GPUTexture* texture, const CreateInfo& info);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    static std::optional<Texture> Create(GraphicsDevice& device, const CreateInfo& info, const char* debugName = nullptr);

    [[nodiscard]]
    bool IsValid() const { return m_texture != nullptr; }
    [[nodiscard]]
    SDL_GPUTexture* NativeHandle() const { return m_texture; }
    [[nodiscard]]
    uint32_t Width() const { return m_width; }
    [[nodiscard]]
    uint32_t Height() const { return m_height; }
    [[nodiscard]]
    uint32_t Layers() const { return m_layers; }
    [[nodiscard]]
    uint32_t MipLevels() const { return m_mipLevels; }

private:
    void Release();

    GraphicsDevice* m_device = nullptr;
    SDL_GPUTexture* m_texture = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_layers = 1;
    uint32_t m_mipLevels = 1;
};
} // Lutum

#endif //LUTUM_TEXTURE_HPP
