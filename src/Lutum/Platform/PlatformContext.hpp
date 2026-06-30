/*
* File: PlatformContext.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUMENGINE_PLATFORMCONTEXT_HPP
#define LUTUMENGINE_PLATFORMCONTEXT_HPP

namespace Lutum {
class PlatformContext {
public:
    PlatformContext();
    ~PlatformContext();

    PlatformContext(const PlatformContext&) = delete;
    PlatformContext& operator=(const PlatformContext&) = delete;

    bool IsValid() const { return m_valid; }

private:
    bool m_valid = false;
};
} // Lutum

#endif //LUTUMENGINE_PLATFORMCONTEXT_HPP
