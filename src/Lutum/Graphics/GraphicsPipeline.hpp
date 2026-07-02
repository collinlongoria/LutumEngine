/*
* File: GraphicsPipeline.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_GRAPHICSPIPELINE_HPP
#define LUTUM_GRAPHICSPIPELINE_HPP
#include <optional>

struct SDL_GPUGraphicsPipeline;

namespace Lutum {

class GraphicsDevice;
class Shader;

enum class PrimitiveType {
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    LINE_LIST,
    LINE_STRIP,
    POINT_LIST
};

class GraphicsPipeline {
public:
    struct CreateInfo {
        Shader* vertexShader = nullptr;
        Shader* fragmentShader = nullptr;
        PrimitiveType primitiveType = PrimitiveType::TRIANGLE_LIST;
        // TODO: color target format is currently pulled from swapchain
    };

    GraphicsPipeline() = default;
    GraphicsPipeline(GraphicsDevice& device, SDL_GPUGraphicsPipeline* pipeline);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline && other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline && other) noexcept;

    static std::optional<GraphicsPipeline> Create(GraphicsDevice& device, const CreateInfo& info);

    [[nodiscard]]
    bool IsValid() const { return m_pipeline != nullptr; }
    [[nodiscard]]
    SDL_GPUGraphicsPipeline* NativeHandle() const { return m_pipeline; }

private:
    void Release();

    GraphicsDevice* m_device = nullptr;
    SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
};
} // Lutum

#endif //LUTUM_GRAPHICSPIPELINE_HPP
