/*
* File: DebugUI.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_DEBUGUI_HPP
#define LUTUM_DEBUGUI_HPP
#include "Lutum/Graphics/RenderTarget.hpp"


union SDL_Event;
struct SDL_GPUCommandBuffer;
struct SDL_GPURenderPass;
struct SDL_GPUSampler;

namespace Lutum {

class GraphicsDevice;

/*
 * Lifecycle + render for ImGui
 *
 * NOTE: All functions are safe no-ops when LUTUM_ENABLE_DEBUG_UI is off or Initialize was not called
 */
namespace Debug::UI {
    bool Initialize(GraphicsDevice& device);
    void Shutdown();

    [[nodiscard]]
    bool IsInitialized();

    // Feed every SDL event
    void ProcessEvent(const SDL_Event& event);

    // Start a UI frame
    void BeginFrame();

    // Finalize draw data and upload buffers
    void PrepareRender(SDL_GPUCommandBuffer* cmd);

    // Record UI draws into an open render pass
    void Render(SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass);
} // Debug::UI
} // Lutum

#endif //LUTUM_DEBUGUI_HPP
