/*
* File: PayloadIO.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/9/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_PAYLOADIO_HPP
#define LUTUM_PAYLOADIO_HPP
#include <cstdint>
#include <cstring>
#include <vector>

// TODO: cleanup Snapshot.cpp
namespace Lutum::AssetIO {

struct ByteWriter {
    std::vector<uint8_t> buf;

    void Bytes(const void* p, size_t n) {
        const auto* b = static_cast<const uint8_t*>(p);
        buf.insert(buf.end(), b, b + n);
    }
    void U32(uint32_t v) { Bytes(&v, sizeof(v)); }
    void U64(uint64_t v) { Bytes(&v, sizeof(v)); }
};

struct ByteReader {
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t pos = 0;

    bool Bytes(void* out, size_t n) {
        if (n > size - pos)
            return false;
        std::memcpy(out, data + pos, n);
        pos += n;
        return true;
    }

    // zero-copy view; nullptr on overrun
    [[nodiscard]]
    const uint8_t* View(size_t n) {
        if (n > size - pos)
            return nullptr;
        const uint8_t* p = data + pos;
        pos += n;
        return p;
    }

    template <typename T>
    bool Read(T& out) { return Bytes(&out, sizeof(T)); }
};

} // Lutum::AssetIO

#endif //LUTUM_PAYLOADIO_HPP
