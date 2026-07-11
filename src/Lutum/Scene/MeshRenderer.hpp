/*
* File: MeshRenderer.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/11/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_MESHRENDERER_HPP
#define LUTUM_MESHRENDERER_HPP
#include <array>
#include <cstddef>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/ECS/Reflect.hpp"

namespace Lutum {

struct MeshRenderer {
    static constexpr const char* kCuriaName = "MeshRenderer";

    AssetID mesh{}; // StaticMesh; null = nothing drawn
    AssetID material{}; // Material; null = DefaultMaterial fallback

    static constexpr auto CuriaFields() {
        using namespace Curia;
        return std::array{
            FieldInfo{"mesh",     offsetof(MeshRenderer, mesh),     FieldType::AssetRef, 1},
            FieldInfo{"material", offsetof(MeshRenderer, material), FieldType::AssetRef, 1},
        };
    }
};

} // Lutum
#endif //LUTUM_MESHRENDERER_HPP
