/*
* File: AssetImport.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ASSETIMPORT_HPP
#define LUTUM_ASSETIMPORT_HPP
#include <string>

namespace Lutum::AssetImport {

bool ImportMeshFile(const std::string& absolutePath);
bool ImportTextureFile(const std::string& absolutePath);

} // Lutum::AssetImport
#endif //LUTUM_ASSETIMPORT_HPP
