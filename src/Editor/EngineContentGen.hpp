/*
* File: EngineContentGen.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/10/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ENGINECONTENTGEN_HPP
#define LUTUM_ENGINECONTENTGEN_HPP

namespace Lutum {

// DEV-ONLY: (re)generates committed engine content under EngineAssets/(Cube/Plane primitives, DefaultTexture, DefaultMaterial)
// Writes via absolute path; the ONE sanctioned bypass of the /Engine/ write guard
// Regeneration preserves existing UUIDs so committed references survive
// Caller should Assets::Rescan() on success
bool GenerateEngineContent();

} // Lutum
#endif //LUTUM_ENGINECONTENTGEN_HPP
