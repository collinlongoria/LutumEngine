/*
* File: ArchetypesPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ARCHETYPESPANEL_HPP
#define LUTUM_ARCHETYPESPANEL_HPP

namespace Lutum {
struct EditorContext;
namespace Curia { class Registry; }

class ArchetypesPanel {
public:
    void Draw(Curia::Registry& registry, EditorContext& context);
};
} // Lutum

#endif
