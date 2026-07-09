/*
* File: AssetHeader.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ASSETHEADER_HPP
#define LUTUM_ASSETHEADER_HPP
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"

namespace Lutum {

static_assert(std::endian::native == std::endian::little,
    "asset header format is little-endian raw; add byte swapping before porting to a BE target");

// TODO: reminding myself to move all notes to docs
// [0] u32 magic 'LAST'
// [4] u32 container version
// [8] u64 type key
// [16] u64 UUID
// [24] u64 reserved
struct AssetHeader {
    static constexpr uint32_t kMagic = 0x5453414Cu;
    static constexpr uint32_t kContainerVersion = 1;
    static constexpr size_t kSize = 32;

    uint32_t containerVersion = kContainerVersion;
    StableKey typeKey = 0;
    AssetID id{};

    [[nodiscard]]
    std::array<uint8_t, kSize> Encode() const {
        std::array<uint8_t, kSize> out{};
        std::memcpy(out.data() + 0, &kMagic, 4);
        std::memcpy(out.data() + 4, &containerVersion, 4);
        std::memcpy(out.data() + 8, &typeKey, 8);
        std::memcpy(out.data() + 16, &id.value, 8);
        return out;
    }

    // nullopt on short input, bad magic, or unsupported container version
    [[nodiscard]]
    static std::optional<AssetHeader> Decode(std::span<const uint8_t> bytes) {
        if (bytes.size() < kSize)
            return std::nullopt;

        uint32_t magic = 0;
        std::memcpy(&magic, bytes.data(), 4);
        if (magic != kMagic)
            return std::nullopt;

        AssetHeader header;
        std::memcpy(&header.containerVersion, bytes.data() + 4, 4);
        if (header.containerVersion != kContainerVersion)
            return std::nullopt;

        std::memcpy(&header.typeKey, bytes.data() + 8, 8);
        std::memcpy(&header.id.value, bytes.data() + 16, 8);
        return header;
    }
};

}

#endif //LUTUM_ASSETHEADER_HPP
