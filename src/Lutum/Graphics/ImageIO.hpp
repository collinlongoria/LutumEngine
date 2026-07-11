/*
* File: ImageIO.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_IMAGEIO_HPP
#define LUTUM_IMAGEIO_HPP
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Lutum {

struct ImageData {
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> pixels; // always RGBA8, tightly packed
};

namespace ImageIO {
    // Loads via FileSystem
    // Forces RGBA8.
    [[nodiscard]]
    std::optional<ImageData> Load(std::string_view virtualPath);

    // Decodes an in-memory image (Texture asset payloads)
    // Forces RGBA8
    [[nodiscard]]
    std::optional<ImageData> LoadFromMemory(std::span<const uint8_t> bytes);
} // ImageIO
} // Lutum

#endif //LUTUM_IMAGEIO_HPP
