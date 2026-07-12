/*
* File: AssetPicker.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/12/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ASSETPICKER_HPP
#define LUTUM_ASSETPICKER_HPP
#include "Lutum/Assets/AssetID.hpp"
#include "Lutum/Core/Hash.hpp"

namespace Lutum {

bool DrawAssetPicker(const char* label, AssetID& value, StableKey typeFilter);

} // Lutum
#endif //LUTUM_ASSETPICKER_HPP
