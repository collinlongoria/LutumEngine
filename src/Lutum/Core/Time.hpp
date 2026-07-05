/*
* File: Time.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_TIME_HPP
#define LUTUM_TIME_HPP
#include <cstdint>

namespace Lutum {
class Time {
public:
    static constexpr const char* kCuriaName = "Time";
    
    Time();

    // Called once per frame, at the top of the game loop
    void Tick();

    [[nodiscard]]
    float DeltaSeconds() const { return m_deltaSeconds; }
    [[nodiscard]]
    double ElapsedSeconds() const { return m_elapsedSeconds; }

private:
    uint64_t m_lastTicksNS = 0;
    float m_deltaSeconds = 0.0f;
    double m_elapsedSeconds = 0.0;
};
} // Lutum

#endif //LUTUM_TIME_HPP
