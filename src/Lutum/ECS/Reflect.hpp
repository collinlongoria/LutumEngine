/*
* File: Reflect.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_REFLECT_HPP
#define LUTUM_CURIA_REFLECT_HPP
#include <cstddef>
#include <cstdint>

namespace Lutum::Curia {

// TODO: consider solution for getters/setters, attributes, ranges, categories, nested user structs, and containers
enum class FieldType : uint8_t {
    F32, I32, U32, U16, U8, Bool,
    Vec2, Vec3, Vec4, Quat,
    EntityRef,
};

[[nodiscard]]
constexpr size_t FieldTypeSize(FieldType type) {
    switch (type) {
        case FieldType::F32:
        case FieldType::I32:
        case FieldType::U32:       return 4;
        case FieldType::U16:       return 2;
        case FieldType::U8:
        case FieldType::Bool:      return 1;
        case FieldType::Vec2:      return 8;
        case FieldType::Vec3:      return 12;
        case FieldType::Vec4:
        case FieldType::Quat:      return 16;
        case FieldType::EntityRef: return 8;
    }
    return 0;
}

static_assert(sizeof(bool) == 1, "FieldType::Bool assumes 1-byte bool");

// TODO: documentation
struct FieldInfo {
    const char* name = nullptr;
    uint32_t offset = 0;
    FieldType type = FieldType::F32;
    uint32_t count = 1;
};

} // Lutum::Curia

#endif //LUTUM_CURIA_REFLECT_HPP
