/*
* File: ShaderCompiler.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/2/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_SHADERCOMPILER_HPP
#define LUTUM_SHADERCOMPILER_HPP
#include <optional>

namespace Lutum {

class Shader;
enum class ShaderStage;
class GraphicsDevice;

class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler();

    bool Initialize();
    void Shutdown();

    std::optional<Shader> LoadHLSL(GraphicsDevice& device, const char* hlslSource, ShaderStage stage);

    ShaderCompiler(const ShaderCompiler&) = delete;
    ShaderCompiler& operator=(const ShaderCompiler&) = delete;

private:
};
} // Lutum

#endif //LUTUM_SHADERCOMPILER_HPP
