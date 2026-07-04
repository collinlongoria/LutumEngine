/*
* File: Scheduler.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_SCHEDULER_HPP
#define LUTUM_CURIA_SCHEDULER_HPP
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <string_view>

#include "Lutum/ECS/CommandBuffer.hpp"
#include "Lutum/ECS/Component.hpp"
#include "Lutum/ECS/Registry.hpp"
#include "Lutum/ECS/Resource.hpp"

namespace Lutum::Curia {

/*
* Systems declare what they read and write (components and resources)
* the scheduler derives a phase layout where no two concurrently-running systems conflict
* Within a phase, systems run in parallel on the job system; between phases, command buffers are flushed sequentially
*
* Usage example:
*   scheduler.AddSystem("Movement")
*       .Reads<Velocity>()
*       .Writes<Position>()
*       .After("InputCapture") // optional explicit ordering
*       .Execute([&](Registry& r, CommandBuffer& cmd) {
*           s_moveQuery.Refresh(r);
*           s_moveQuery.Each([](Position& p, Velocity& v) { ... });
*       });
*
* NOTES:
* - Only touch components you declared
* - No direct structural changes; use the CommandBuffer parameter
* - Query::Refresh inside a system is safe: archetypes can't be created mid-phase
*/
class Scheduler {
public:
    explicit Scheduler(Registry& registry) : m_registry(&registry) {}

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    class SystemBuilder;

    // Name must be unique
    // Builder is valid until the next AddSystem call
    SystemBuilder AddSystem(std::string name);

    // Derives phases
    // Called lazily by Run if needed; call explicitly if you want LogGraph before the first frame
    void Compile();

    // Executes one full pass of all phases
    void Run();

    void LogGraph() const;

    [[nodiscard]]
    size_t PhaseCount() const {
        return m_phases.size();
    }

    [[nodiscard]]
    size_t PhaseSize(size_t phaseIndex) const {
        LUTUM_ASSERT(phaseIndex < m_phases.size(), "phase index {} out of range", phaseIndex);
        return m_phases[phaseIndex].size();
    }

    [[nodiscard]]
    int PhaseOf(std::string_view systemName) const {
        for (size_t phaseIndex = 0; phaseIndex < m_phases.size(); ++phaseIndex) {
            for (const SystemNode* system : m_phases[phaseIndex]) {
                if (system->name == systemName) {
                    return static_cast<int>(phaseIndex);
                }
            }
        }

        return -1;
    }

    [[nodiscard]]
    bool PhaseContains(size_t phaseIndex, std::string_view systemName) const {
        if (phaseIndex >= m_phases.size()) {
            return false;
        }

        for (const SystemNode* system : m_phases[phaseIndex]) {
            if (system->name == systemName) {
                return true;
            }
        }

        return false;
    }

    // Compiled phase layout as names, for tooling/UI
    // Empty if not compiled
    [[nodiscard]]
    std::vector<std::vector<std::string_view>> PhaseNames() const {
        std::vector<std::vector<std::string_view>> result;
        result.reserve(m_phases.size());
        for (const auto& phase : m_phases) {
            auto& names = result.emplace_back();
            names.reserve(phase.size());
            for (const SystemNode* system : phase) {
                names.push_back(system->name);
            }
        }
        return result;
    }

private:
    struct SystemNode {
        std::string name;
        std::vector<ComponentID> reads;
        std::vector<ComponentID> writes;
        std::vector<ResourceID> resourceReads;
        std::vector<ResourceID> resourceWrites;
        std::vector<std::string> after;
        std::function<void(Registry&, CommandBuffer&)> exec;
        CommandBuffer cmd;
        int phase = -1;
    };

    [[nodiscard]]
    static bool Conflicts(const SystemNode& a, const SystemNode& b);
    [[nodiscard]]
    const SystemNode* FindByName(const std::string& name, size_t limit) const;

    Registry* m_registry = nullptr;
    std::vector<std::unique_ptr<SystemNode>> m_systems;
    std::vector<std::vector<SystemNode*>> m_phases;
    bool m_compiled = false;

public:
    class SystemBuilder {
    public:
        explicit SystemBuilder(SystemNode& node) : m_node(&node) {}

        template <IsValidComponent T>
        SystemBuilder& Reads() {
            m_node->reads.push_back(ComponentType<T>::Id());
            return *this;
        }

        template <IsValidComponent T>
        SystemBuilder& Writes() {
            m_node->writes.push_back(ComponentType<T>::Id());
            return *this;
        }

        template <typename T>
        SystemBuilder& ReadsResource() {
            m_node->resourceReads.push_back(ResourceType<T>::Id());
            return *this;
        }

        template <typename T>
        SystemBuilder& WritesResource() {
            m_node->resourceWrites.push_back(ResourceType<T>::Id());
            return *this;
        }

        SystemBuilder& After(std::string systemName) {
            m_node->after.push_back(std::move(systemName));
            return *this;
        }

        template <typename Func>
        void Execute(Func&& fn) {
            m_node->exec = std::forward<Func>(fn);
        }

    private:
        SystemNode* m_node;
    };
};
} // Lutum::Curia


#endif //LUTUM_CURIA_SCHEDULER_HPP
