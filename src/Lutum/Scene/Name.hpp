/*
* File: Name.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/6/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_NAME_HPP
#define LUTUM_NAME_HPP
#include <array>
#include <cstddef>
#include <cstdio>

#include "Lutum/ECS/Reflect.hpp"

namespace Lutum {

struct Name {
    static constexpr const char* kCuriaName = "Name";
    static constexpr size_t kCapacity = 32;

    char value[kCapacity] = {};

    static constexpr auto CuriaFields() {
        using namespace Curia;
        return std::array{
            FieldInfo{"value", offsetof(Name, value), FieldType::Char, kCapacity},
        };
    }
};

// Truncates to fit
// Always NUL-terminated
inline Name MakeName(const char* text) {
    Name n{};
    std::snprintf(n.value, Name::kCapacity, "%s", text != nullptr ? text : "");
    return n;
}

} // Lutum

#endif //LUTUM_NAME_HPP
