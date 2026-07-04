/*
* File: Jobs.hpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#ifndef LUTUM_JOBS_HPP
#define LUTUM_JOBS_HPP
#include <atomic>
#include <cstdint>
#include <functional>

namespace Lutum {

/*
 * Tracks completion of a batch of jobs
 * Pass the same counter to multiple Execute/ParallelFor calls to await them as a group
 * Reusable after Wait returns
 *
 * NOTE: Not copyable/moveable: jobs hold a pointer to it, so it must outlive them
 */
class JobCounter {
public:
    JobCounter() = default;

    JobCounter(const JobCounter&) = delete;
    JobCounter& operator=(const JobCounter&) = delete;
    JobCounter(JobCounter&&) = delete;
    JobCounter& operator=(JobCounter&&) = delete;

    [[nodiscard]]
    bool IsDone() const { return m_pending.load(std::memory_order_acquire) == 0; }

private:
    friend struct JobsInternal;
    std::atomic<uint32_t> m_pending{0};
};

namespace Jobs {
    // threadCount 0 = auto (hardware threads -1 , min 1)
    // Call after Log::Initialize so worker startup is logged!!!
    bool Initialize(uint32_t threadCount = 0);

    // Waits for in-flight jobs, then joins all workers
    void Shutdown();

    [[nodiscard]]
    uint32_t WorkerCount();

    // Enqueue a single job
    // If counter is non-null it is incremented now and decremented when the job finishes
    void Execute(std::function<void()> job, JobCounter* counter = nullptr);

    // Splits [0, count) into ranges of at most grainSize and runs fn(begin, end) across the pool
    // BLOCKS until all ranges complete
    // grainSize 0 = auto
    void ParallelFor(uint32_t count, uint32_t grainSize, const std::function<void(uint32_t begin, uint32_t end)>& fn);

    // Block until counter hits zero
    // The calling thread executes queued jobs while waiting, so it is safe to wait from inside a job
    void Wait(JobCounter& counter);
} // Jobs
} // Lutum

#endif //LUTUM_JOBS_HPP
