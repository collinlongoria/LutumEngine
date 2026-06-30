/*
* File: Application.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 6/30/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_APP_HPP
#define LUTUM_APP_HPP

namespace Lutum {
class Application {
public:
    Application() = default;
    ~Application() = default;

    bool Initialize();
    void Run();
};
} // Lutum

#endif //LUTUM_APP_HPP
