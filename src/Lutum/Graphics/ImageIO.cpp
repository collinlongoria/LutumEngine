/*
* File: ImageIO.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Graphics/ImageIO.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include <stb_image.h>

#include "Lutum/Core/FileSystem.hpp"
#include "Lutum/Core/Log.hpp"

namespace Lutum::ImageIO {

std::optional<ImageData> Load(std::string_view virtualPath) {
    auto bytes = FileSystem::ReadBytes(virtualPath);
    if (!bytes)
        return std::nullopt;

    int width = 0, height = 0, channels = 0;
    stbi_uc* pixels = stbi_load_from_memory(
        bytes->data(), static_cast<int>(bytes->size()),
        &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        LUTUM_ERROR("ImageIO: failed to decode '{}': {}", virtualPath, stbi_failure_reason());
        return std::nullopt;
    }

    ImageData image;
    image.width = static_cast<uint32_t>(width);
    image.height = static_cast<uint32_t>(height);
    image.pixels.assign(pixels, pixels + static_cast<size_t>(width) * height * 4);

    stbi_image_free(pixels);
    return image;
}
} // Lutum::ImageIO