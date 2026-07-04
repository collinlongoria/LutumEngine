/*
* File: CameraSystems.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CAMERASYSTEMS_HPP
#define LUTUM_CAMERASYSTEMS_HPP

namespace Lutum {
namespace Curia { class Scheduler; }

// Registers "FlyCam" and "CameraAspect".
void RegisterCameraSystems(Curia::Scheduler& scheduler);
} // Lutum

#endif //LUTUM_CAMERASYSTEMS_HPP
