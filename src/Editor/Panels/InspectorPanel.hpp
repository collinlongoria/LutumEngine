/*
* File: InspectorPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_INSPECTORPANEL_HPP
#define LUTUM_INSPECTORPANEL_HPP
#include <cstddef>

namespace Lutum {
struct EditorContext;
namespace Curia { class Registry; struct FieldInfo; }

class InspectorPanel {
public:
    void Draw(Curia::Registry& registry, EditorContext& context);

private:
    static void DrawField(const Curia::FieldInfo& field, std::byte* base);
};
} // Lutum

#endif
