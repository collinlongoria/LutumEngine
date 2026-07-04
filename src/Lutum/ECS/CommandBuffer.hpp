/*
* File: CommandBuffer.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_COMMANDBUFFER_HPP
#define LUTUM_CURIA_COMMANDBUFFER_HPP
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "Lutum/Core/Log.hpp"
#include "Lutum/ECS/Component.hpp"
#include "Lutum/ECS/Entity.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace Lutum::Curia {

class CommandBuffer {
public:
    CommandBuffer() = default;

    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;

    CommandBuffer(CommandBuffer&&) = default;
    CommandBuffer& operator=(CommandBuffer&&) = default;

    // Placeholder entity, remapped to a real one at Execute
    [[nodiscard]]
    Entity CreateDeferred() {
        const uint32_t tempIndex = m_tempCount++;
        LUTUM_ASSERT(tempIndex != std::numeric_limits<uint32_t>::max(), "deferred create overflow");
        return EntityTraits::Make(tempIndex, TEMP_GENERATION);
    }

    template <IsValidComponent T>
    void Add(Entity e, T componentData = {}) {
        GetQueue<T>().adds.emplace_back(e, std::move(componentData));
    }

    template <IsValidComponent T>
    void Remove(Entity e) {
        GetQueue<T>().removes.push_back(e);
    }

    void Destroy(Entity e) {
        m_destroys.push_back(e);
    }

    void Execute(Registry& registry) {
        // Materialize placeholders first so every queue can resolve them.
        std::vector<Entity> remap(m_tempCount);
        for (uint32_t i = 0; i < m_tempCount; ++i) {
            remap[i] = registry.Create();
        }

        for (auto& queue : m_queues) {
            if (!queue->Empty()) {
                queue->Execute(registry, remap);
            }
        }

        for (Entity e : m_destroys) {
            registry.Destroy(Resolve(e, remap));
        }

        Clear();
    }

    void Clear() {
        for (auto& queue : m_queues) {
            queue->Clear();
        }
        m_destroys.clear();
        m_tempCount = 0;
    }

    [[nodiscard]]
    bool Empty() const {
        if (m_tempCount > 0 || !m_destroys.empty())
            return false;

        for (const auto& queue : m_queues) {
            if (!queue->Empty())
                return false;
        }
        return true;
    }

private:
    // Placeholder marker: generation all-ones
    // TODO: a real generation can reach this after 2^32 recycles of one index
    static constexpr uint32_t TEMP_GENERATION = 0xFFFFFFFFu;

    [[nodiscard]]
    static bool IsTemp(Entity e) {
        return e != INVALID_ENTITY && EntityTraits::Generation(e) == TEMP_GENERATION;
    }

    [[nodiscard]]
    static Entity Resolve(Entity e, const std::vector<Entity>& remap) {
        return IsTemp(e) ? remap[EntityTraits::Index(e)] : e;
    }

    struct IQueue {
        virtual ~IQueue() = default;
        virtual void Execute(Registry& registry, const std::vector<Entity>& remap) = 0;
        virtual void Clear() = 0;
        [[nodiscard]] virtual bool Empty() const = 0;
    };

    template <IsValidComponent T>
    struct TypedQueue final : IQueue {
        std::vector<std::pair<Entity, T>> adds;
        std::vector<Entity> removes;

        void Execute(Registry& registry, const std::vector<Entity>& remap) override {
            for (auto& [e, data] : adds) {
                registry.Add<T>(Resolve(e, remap), std::move(data));
            }
            for (Entity e : removes) {
                registry.Remove<T>(Resolve(e, remap));
            }
        }

        void Clear() override {
            adds.clear();
            removes.clear();
        }

        [[nodiscard]]
        bool Empty() const override {
            return adds.empty() && removes.empty();
        }
    };

    template <IsValidComponent T>
    TypedQueue<T>& GetQueue() {
        const ComponentID cid = ComponentType<T>::Id();

        if (cid >= m_componentToQueue.size()) {
            m_componentToQueue.resize(cid + 1, std::numeric_limits<size_t>::max());
        }

        if (m_componentToQueue[cid] == std::numeric_limits<size_t>::max()) {
            m_componentToQueue[cid] = m_queues.size();
            m_queues.push_back(std::make_unique<TypedQueue<T>>());
        }

        return static_cast<TypedQueue<T>&>(*m_queues[m_componentToQueue[cid]]);
    }

    std::vector<std::unique_ptr<IQueue>> m_queues;
    std::vector<size_t> m_componentToQueue;
    std::vector<Entity> m_destroys;
    uint32_t m_tempCount = 0;

};

} // Lutum::Curia

#endif //LUTUM_CURIA_COMMANDBUFFER_HPP
