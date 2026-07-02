/*
* File: Shader.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUMENGINE_SHADER_HPP
#define LUTUMENGINE_SHADER_HPP

struct SDL_GPUShader;

namespace Lutum {

class GraphicsDevice;

enum class ShaderStage {
    VERTEX,
    FRAGMENT,
    COMPUTE
};

class Shader {
public:
    Shader() = default;
    Shader(GraphicsDevice& device, SDL_GPUShader* shader);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    [[nodiscard]]
    bool IsValid() const { return m_shader != nullptr; }
    [[nodiscard]]
    SDL_GPUShader* NativeHandle() const { return m_shader; }

private:
    void Release();

    GraphicsDevice* m_device = nullptr;
    SDL_GPUShader* m_shader = nullptr;
};
} // Lutum

#endif //LUTUMENGINE_SHADER_HPP
