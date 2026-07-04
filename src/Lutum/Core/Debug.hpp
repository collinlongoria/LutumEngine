/*
* File: Debug.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_DEBUG_HPP
#define LUTUM_DEBUG_HPP
#include <cstdint>

namespace Lutum::Debug {
    void Break();
    void SleepMilliseconds(uint32_t milliseconds);
}

#endif //LUTUM_DEBUG_HPP
