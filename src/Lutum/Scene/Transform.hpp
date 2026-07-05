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
#include "Lutum/Core/Math.hpp"

namespace Lutum {

struct Transform {
    static constexpr const char* kCuriaName = "Transform";

    Vec3 position = Vec3(0.0f);
    Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 scale = Vec3(1.0f);
};
} // Lutum

#endif //LUTUM_TRANSFORM_HPP
