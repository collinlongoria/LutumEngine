/*
* File: FlyCameraController.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_FLYCAMERACONTROLLER_HPP
#define LUTUM_FLYCAMERACONTROLLER_HPP

namespace Lutum {

class Camera;
class Input;

class FlyCameraController {
public:
    FlyCameraController() = default;

    void Update(Camera& camera, const Input& input, float deltaSeconds);

    void SetMoveSpeed(float unitsPerSecond) { m_moveSpeed = unitsPerSecond; }
    void SetLookSensitivity(float radiansPerPixel) { m_lookSensitivity = radiansPerPixel; }

private:
    float m_moveSpeed = 5.0f;
    float m_sprintMultiplier = 4.0f;
    float m_lookSensitivity = 0.0025f;
};
} // Lutum

#endif //LUTUM_FLYCAMERACONTROLLER_HPP
