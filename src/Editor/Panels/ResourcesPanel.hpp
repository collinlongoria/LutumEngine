/*
* File: ResourcesPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_RESOURCESPANEL_HPP
#define LUTUM_RESOURCESPANEL_HPP

namespace Lutum {
struct EditorContext;
namespace Curia { class Registry; }

class ResourcesPanel {
public:
    void Draw(Curia::Registry& registry, EditorContext& context);
};
} // Lutum

#endif
