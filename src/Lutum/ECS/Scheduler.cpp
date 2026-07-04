/*
* File: Scheduler.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/ECS/Scheduler.hpp"

#include <algorithm>

#include "Lutum/Core/Jobs.hpp"
#include "Lutum/Core/Log.hpp"


namespace Lutum::Curia {

Scheduler::SystemBuilder Scheduler::AddSystem(std::string name) {
    LUTUM_ASSERT(FindByName(name, m_systems.size()) == nullptr, "duplicate system name '{}'", name);

    m_compiled = false;

    auto node = std::make_unique<SystemNode>();
    node->name = std::move(name);
    m_systems.push_back(std::move(node));
    return SystemBuilder(*m_systems.back());
}

const Scheduler::SystemNode* Scheduler::FindByName(const std::string& name, size_t limit) const {
    for (size_t i = 0; i < limit && i < m_systems.size(); ++i) {
        if (m_systems[i]->name == name)
            return m_systems[i].get();
    }
    return nullptr;
}

bool Scheduler::Conflicts(const SystemNode& a, const SystemNode& b) {
    const auto contains = [](const auto& vec, auto id) {
        return std::find(vec.begin(), vec.end(), id) != std::end(vec);
    };

    // Components: a's reads vs b's writes, a's writes vs b's reads/writes
    for (ComponentID r : a.reads) {
        if (contains(b.writes, r)) return true;
    }
    for (ComponentID w : a.writes) {
        if (contains(b.reads, w) || contains(b.writes, w)) return true;
    }

    // Resources: same rules
    for (ResourceID r : a.resourceReads) {
        if (contains(b.resourceWrites, r)) return true;
    }
    for (ResourceID w : a.resourceWrites) {
        if (contains(b.resourceReads, w) || contains(b.resourceWrites, w)) return true;
    }

    return false;
}

void Scheduler::Compile() {
    m_phases.clear();

    for (size_t i = 0; i < m_systems.size(); ++i) {
        SystemNode* current = m_systems[i].get();
        LUTUM_ASSERT(current->exec != nullptr, "system '{}' has no Execute function", current->name);

        int targetPhase = 0;

        // Data conflicts with earlier systems push us to a later phase
        for (size_t j = 0; j < i; ++j) {
            const SystemNode* prev = m_systems[j].get();
            if (Conflicts(*current, *prev)) {
                targetPhase = std::max(targetPhase, prev->phase + 1);
            }
        }

        // Explicit ordering: must run in a phase strictly after the named system
        // Referenced system must have been added earlier
        for (const std::string& depName : current->after) {
            const SystemNode* dep = FindByName(depName, i);
            LUTUM_ASSERT(dep != nullptr,
                         "system '{}': After(\"{}\") refers to an unknown or later-added system",
                         current->name, depName);
            if (dep) {
                targetPhase = std::max(targetPhase, dep->phase + 1);
            }
        }

        current->phase = targetPhase;
        if (static_cast<size_t>(targetPhase) >= m_phases.size()) {
            m_phases.resize(targetPhase + 1);
        }
        m_phases[targetPhase].push_back(current);
    }

    m_compiled = true;
}

void Scheduler::Run() {
    if (!m_compiled)
        Compile();

    for (auto& phase : m_phases) {
        // Systems within a phase have no declared conflicts: run in parallel
        // Single-system phases skip the queue round trip
        if (phase.size() == 1) {
            phase[0]->exec(*m_registry, phase[0]->cmd);
        }
        else {
            JobCounter counter;
            for (SystemNode* system : phase) {
                Jobs::Execute([this, system] {
                    system->exec(*m_registry, system->cmd);
                }, &counter);
            }
            Jobs::Wait(counter);
        }

        // Structural changes happen here in system insertion order
        // Observers fire during this window
        for (SystemNode* system : phase) {
            if (!system->cmd.Empty()) {
                system->cmd.Execute(*m_registry);
            }
        }
    }
}

void Scheduler::LogGraph() const {
    LUTUM_INFO("--- System Dependency Graph ---");
    for (size_t i = 0; i < m_phases.size(); ++i) {
        LUTUM_INFO("Phase {}:", i);
        for (const SystemNode* system : m_phases[i]) {
            LUTUM_INFO("  [{}] (reads {} / writes {} components, {} / {} resources)",
                       system->name,
                       system->reads.size(), system->writes.size(),
                       system->resourceReads.size(), system->resourceWrites.size());
        }
    }
    LUTUM_INFO("-------------------------------");
}
} // Lutum::Curia