/*
* File: SystemsPanel.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/5/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_SYSTEMSPANEL_HPP
#define LUTUM_SYSTEMSPANEL_HPP

namespace Lutum {
struct EditorContext;
namespace Curia { class Scheduler; }

class SystemsPanel {
public:
    void Draw(Curia::Scheduler& scheduler, EditorContext& context);
};
} // Lutum

#endif //LUTUM_SYSTEMSPANEL_HPP
