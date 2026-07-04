/*
* File: RenderTarget.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_RENDERTARGET_HPP
#define LUTUM_RENDERTARGET_HPP
#include <cstdint>
#include <optional>

#include "Lutum/Core/Math.hpp"

struct SDL_GPUTexture;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;

namespace Lutum {

class GraphicsDevice;

enum class TargetFormat {
    SWAPCHAIN, // color matches the window swapchain
    RGBA8_UNORM, // offscreen color
    DEPTH_DEFAULT // D32_FLOAT, falling back to D24_UNORM if unsupported
};

/*
 * A color target + optional depth target, sized together and recreated together on resize
 *
 * Two modes:
 * - Swapchain-backed: does NOT own a color texture
 *      - The caller acquires the swapchain texture each frame and passes it to BeginRenderPass; Owns Depth
 * - Offscreen: owns both color and depth
 *      - Color is created with sampler usage so it can be drawn as a UI image
 */
class RenderTarget {
public:
    struct CreateInfo {
        uint32_t width = 0;
        uint32_t height = 0;
        bool offscreen = false;
        TargetFormat colorFormat = TargetFormat::SWAPCHAIN;
        bool hasDepth = true;
    };

    RenderTarget() = default;
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    RenderTarget(RenderTarget&& other) noexcept;
    RenderTarget& operator=(RenderTarget&& other) noexcept;

    static std::optional<RenderTarget> Create(GraphicsDevice& device, const CreateInfo& info);

    // Recreates owned textures at new size
    // No-op if size is unchanged
    // NOTE: Safe mid-frame-loop
    bool Resize(uint32_t width, uint32_t height);

    // swapchainTexture is required for swapchain-backed targets and must be null for offscreen targets
    // Returns null on failure
    SDL_GPURenderPass* BeginRenderPass(SDL_GPUCommandBuffer* cmd, SDL_GPUTexture* swapchainTexture, const Vec4& clearColor, float clearDepth = 1.0f);

    [[nodiscard]]
    bool IsValid() const { return m_device != nullptr; }
    [[nodiscard]]
    bool IsOffscreen() const { return m_offscreen; }
    [[nodiscard]]
    uint32_t Width() const { return m_width; }
    [[nodiscard]]
    uint32_t Height() const { return m_height; }
    [[nodiscard]]
    bool HasDepth() const { return m_depthTexture != nullptr; }

    // offscreen only (sampleable)
    [[nodiscard]]
    SDL_GPUTexture* ColorTexture() const { return m_colorTexture; }

    // Actual formats resolved at creation
    // NOTE: pipelines MUST match these
    [[nodiscard]]
    uint32_t NativeColorFormat() const { return m_nativeColorFormat; }
    [[nodiscard]]
    uint32_t NativeDepthFormat() const { return m_nativeDepthFormat; }

private:
    void Release();
    bool CreateTextures();

    GraphicsDevice* m_device = nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_offscreen = false;
    bool m_wantDepth = false;

    SDL_GPUTexture* m_colorTexture = nullptr;
    SDL_GPUTexture* m_depthTexture = nullptr;

    uint32_t m_nativeColorFormat = 0;
    uint32_t m_nativeDepthFormat = 0;
};

} // Lutum

#endif //LUTUM_RENDERTARGET_HPP
