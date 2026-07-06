/*
* File: EntitiesPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_ENTITIESPANEL_HPP
#define LUTUM_ENTITIESPANEL_HPP
#include <string>

#include "Lutum/ECS/Entity.hpp"
#include "Lutum/Scene/Name.hpp"

namespace Lutum {
struct EditorContext;
namespace Curia {
    class Archetype;
    class Registry;
}

std::string SignatureLabel(const Curia::Archetype& arch);

class EntitiesPanel {
public:
    void Draw(Curia::Registry& registry, EditorContext& context);

private:
    void BeginRename(Curia::Entity e, const Name* current);

    Curia::Entity m_renameTarget = Curia::INVALID_ENTITY;
    char m_renameBuffer[Name::kCapacity] = {};
    bool m_renameFocusPending = false;
};
} // Lutum

#endif