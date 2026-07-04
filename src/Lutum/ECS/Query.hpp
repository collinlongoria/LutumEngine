/*
* File: Query.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_CURIA_QUERY_HPP
#define LUTUM_CURIA_QUERY_HPP
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <vector>

#include "Lutum/Core/Jobs.hpp"
#include "Lutum/ECS/Archetype.hpp"
#include "Lutum/ECS/Component.hpp"
#include "Lutum/ECS/Registry.hpp"

namespace Lutum::Curia {

// Filter terms: participate in archetype matching, never appear in callbacks
template<typename T>
struct With {};

template<typename T>
struct Without {};

namespace detail {

    template<typename Term>
    struct TermTraits {
        using Component = Term;
        static constexpr bool IsCallback = true;
        static constexpr bool IsRequired = true;
        static constexpr bool IsExcluded = false;
    };

    template <typename T>
    struct TermTraits<With<T>> {
        using Component = T;
        static constexpr bool IsCallback = false;
        static constexpr bool IsRequired = true;
        static constexpr bool IsExcluded = false;
    };

    template <typename T>
    struct TermTraits<Without<T>> {
        using Component = T;
        static constexpr bool IsCallback = false;
        static constexpr bool IsRequired = false;
        static constexpr bool IsExcluded = true;
    };

    template<typename Term>
    constexpr bool ValidTerm() {
        using Traits = TermTraits<Term>;
        using C = typename Traits::Component;

        if constexpr (Traits::IsCallback) {
            // Empty types carry no data
            return IsValidComponent<C> && !std::is_empty_v<C>;
        }
        else {
            return IsValidComponent<C>;
        }
    }

    // Contributes a 1-tuple for callback terms, an empty tuple for filters;
    // tuple_cat over the pack yields pointers for exactly the callback terms
    template<typename Term>
    auto ArrayFor(const Archetype& arch, Slab* slab) {
        using Traits = TermTraits<Term>;

        if constexpr (Traits::IsCallback) {
            using C = typename Traits::Component;
            return std::make_tuple(reinterpret_cast<C*>(slab->data + arch.ComponentOffset(ComponentType<C>::Id())));
        }
        else {
            return std::tuple<>{};
        }
    }

} // detail

/*
 * Persistent, incrementally-refreshed archetype view
 *
 * Terms are either components (appear in callback, in declaration order) or filters (With<T> / Without<T>, matching only)
 *
 *      Query<Position, Velocity, With<ChunkTag>, Without<Disabled>> q;
 *      q.Refresh(registry);
 *      q.Each([](Position& p, Velocity& v) { ... });
 *
 * NOTES:
 * - Callbacks MUST NOT make structural changes (Add/Remove/Create/Destroy). Defer thru CommandBuffer
 * - ParEach callbacks additionally must be safe to run concurrently with themselves; Only touch the components you ask for
 */
template<typename... Terms>
class Query {
    static_assert(sizeof...(Terms) > 0, "query needs at least one term");
    static_assert((detail::ValidTerm<Terms>() && ...), "query terms must be valid components; empty types (tags) must be wrapped in With<> or Without<>");

public:
    Query() = default;

    // Incremental: only archetypes created since the last Refresh are tested
    void Refresh(const Registry& registry) {
        const auto& archetypes = registry.Archetypes();
        for (size_t i = m_processedCount; i < archetypes.size(); ++i) {
            if (Matches(archetypes[i].get())) {
                m_matches.push_back(archetypes[i].get());
            }
        }
        m_processedCount = archetypes.size();
    }

    template <typename Func>
    void Each(Func&& fn) {
        for (Archetype* arch : m_matches) {
            const uint32_t slabCount = arch->SlabCount();
            for (uint32_t si = 0; si < slabCount; ++si) {
                RunSlab(*arch, arch->GetSlab(si), fn);
            }
        }
    }

    template <typename Func>
    void EachWithEntity(Func&& fn) {
        for (Archetype* arch : m_matches) {
            const uint32_t slabCount = arch->SlabCount();
            for (uint32_t si = 0; si < slabCount; ++si) {
                Slab* slab = arch->GetSlab(si);
                auto arrays = std::tuple_cat(detail::ArrayFor<Terms>(*arch, slab)...);

                const uint32_t count = slab->entityCount;
                std::apply([&](auto*... ptrs) {
                    for (uint32_t i = 0; i < count; ++i) {
                        fn(arch->EntityAt(si, i), ptrs[i]...);
                    }
                }, arrays);
            }
        }
    }

    // Parallel across slabs via the job system
    // Blocks until complete.
    template <typename Func>
    void ParEach(Func&& fn) {
        struct WorkItem {
            Archetype* arch;
            uint32_t slabIndex;
        };

        std::vector<WorkItem> work;
        for (Archetype* arch : m_matches) {
            const uint32_t slabCount = arch->SlabCount();
            for (uint32_t si = 0; si < slabCount; ++si) {
                work.push_back(WorkItem{arch, si});
            }
        }

        if (work.empty())
            return;

        Jobs::ParallelFor(static_cast<uint32_t>(work.size()), 0,
            [&work, &fn, this](uint32_t begin, uint32_t end) {
                for (uint32_t w = begin; w < end; ++w) {
                    RunSlab(*work[w].arch, work[w].arch->GetSlab(work[w].slabIndex), fn);
                }
            });
    }

    [[nodiscard]]
    size_t MatchCount() const { return m_matches.size(); }

private:
    static bool Matches(const Archetype* arch) {
        const bool required = ((
            !detail::TermTraits<Terms>::IsRequired ||
            arch->HasComponent(ComponentType<typename detail::TermTraits<Terms>::Component>::Id())
        ) && ...);

        const bool excluded = ((
            detail::TermTraits<Terms>::IsExcluded &&
            arch->HasComponent(ComponentType<typename detail::TermTraits<Terms>::Component>::Id())
        ) || ...);

        return required && !excluded;
    }

    template <typename Func>
    static void RunSlab(const Archetype& arch, Slab* slab, Func& fn) {
        auto arrays = std::tuple_cat(detail::ArrayFor<Terms>(arch, slab)...);

        const uint32_t count = slab->entityCount;
        std::apply([&](auto*... ptrs) {
            for (uint32_t i = 0; i < count; ++i) {
                fn(ptrs[i]...);
            }
        }, arrays);
    }

    std::vector<Archetype*> m_matches;
    size_t m_processedCount = 0;
};

} // Lutum::Curia

#endif //LUTUM_CURIA_QUERY_HPP
