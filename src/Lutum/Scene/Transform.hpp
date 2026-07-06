/*
* File: Transform.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_TRANSFORM_HPP
#define LUTUM_TRANSFORM_HPP
#include <array>
#include <cstddef>

#include "Lutum/ECS/Reflect.hpp"
#include "Lutum/Core/Math.hpp"

namespace Lutum {

struct Transform {
    static constexpr const char* kCuriaName = "Transform";

    Vec3 position = Vec3(0.0f);
    Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 scale = Vec3(1.0f);

    static constexpr auto CuriaFields() {
        using namespace Curia;
        return std::array{
            FieldInfo{"position", offsetof(Transform, position), FieldType::Vec3, 1},
            FieldInfo{"rotation", offsetof(Transform, rotation), FieldType::Quat, 1},
            FieldInfo{"scale",    offsetof(Transform, scale),    FieldType::Vec3, 1},
        };
    }
};
} // Lutum

#endif //LUTUM_TRANSFORM_HPP
