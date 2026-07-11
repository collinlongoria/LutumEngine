/*
* File: AssetTypeHints.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/11/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ASSETTYPEHINTS_HPP
#define LUTUM_ASSETTYPEHINTS_HPP
#include <array>
#include <string_view>

#include "Lutum/Core/Hash.hpp"

namespace Lutum::AssetTypeHints {

[[nodiscard]]
inline StableKey Lookup(std::string_view componentName, std::string_view fieldName) {
    struct Hint {
        std::string_view component;
        std::string_view field;
        StableKey type;
    };
    // TODO: I do not love this
    static constexpr std::array kHints = {
        Hint{"MeshRenderer", "mesh", HashName("StaticMesh")},
        Hint{"MeshRenderer", "material", HashName("Material")},
    };

    for (const Hint& hint : kHints) {
        if (hint.component == componentName && hint.field == fieldName)
            return hint.type;
    }
    return 0;
}

} // Lutum::AssetTypeHints
#endif //LUTUM_ASSETTYPEHINTS_HPP
