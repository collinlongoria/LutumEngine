/*
* File: Jobs.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include "Lutum/Core/Jobs.hpp"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

#include "Lutum/Core/Log.hpp"

namespace Lutum {
namespace {

    struct Job {
        std::function<void()> fn;
        JobCounter* counter = nullptr;
    };

    struct JobsState {
        std::vector<std::thread> workers;

        std::deque<Job> queue;
        std::mutex queueMutex;
        std::condition_variable queueCV;

        std::atomic<bool> running{false};
    };

    JobsState& State() {
        static JobsState state;
        return state;
    }

} // anonymous namespace

// Friend of JobCounter
// Lets the worker loop touch m_pending without exposing it to public API
struct JobsInternal {
    static void Increment(JobCounter& counter) {
        counter.m_pending.fetch_add(1, std::memory_order_relaxed);
    }

    static void Decrement(JobCounter& counter) {
        counter.m_pending.fetch_sub(1, std::memory_order_release);
    }
};

namespace {

    void RunJob(Job& job) {
        job.fn();
        if (job.counter) {
            JobsInternal::Decrement(*job.counter);
        }
    }

    // Try to pop and run one job
    // Returns false if the queue was empty
    bool TryRunOneJob() {
        JobsState& state = State();

        Job job;
        {
            std::lock_guard lock(state.queueMutex);
            if (state.queue.empty())
                return false;
            job = std::move(state.queue.front());
            state.queue.pop_front();
        }

        RunJob(job);
        return true;
    }

    void WorkerLoop() {
        JobsState& state = State();

        while (true) {
            Job job;
            {
                std::unique_lock lock(state.queueMutex);
                state.queueCV.wait(lock, [&state] {
                    return !state.queue.empty() || !state.running.load(std::memory_order_relaxed);
                });

                if (!state.running.load(std::memory_order_relaxed) && state.queue.empty())
                    return;

                job = std::move(state.queue.front());
                state.queue.pop_front();
            }

            RunJob(job);
        }
    }

} // anonymous namespace

namespace Jobs {
    // TODO: always returns true
    bool Initialize(uint32_t threadCount) {
        JobsState& state = State();

        if (state.running.load(std::memory_order_relaxed)) {
            LUTUM_WARN("Jobs::Initialize called while already running");
            return true;
        }

        if (threadCount == 0) {
            const uint32_t hw = std::thread::hardware_concurrency();
            threadCount = (hw > 1) ? hw - 1 : 1;
        }

        state.running.store(true, std::memory_order_relaxed);
        state.workers.reserve(threadCount);
        for (uint32_t i = 0; i <threadCount; ++i) {
            state.workers.emplace_back(WorkerLoop);
        }

        LUTUM_INFO("Job system initialized with {} worker threads", threadCount);
        return true;
    }

    void Shutdown() {
        JobsState& state = State();

        {
            std::lock_guard lock(state.queueMutex);

            if (!state.running.load(std::memory_order_acquire))
                return;

            state.running.store(false, std::memory_order_release);
        }

        state.queueCV.notify_all();

        for (std::thread& worker : state.workers) {
            if (worker.joinable())
                worker.join();
        }

        state.workers.clear();
        state.queue.clear();

        LUTUM_INFO("Job system shut down");
    }

    uint32_t WorkerCount() {
        return static_cast<uint32_t>(State().workers.size());
    }

    void Execute(std::function<void()> job, JobCounter *counter) {
        JobsState& state = State();

        if (counter) {
            JobsInternal::Increment(*counter);
        }

        {
            std::lock_guard lock(state.queueMutex);

            if (!state.running.load(std::memory_order_acquire)) {
                Job inlineJob{std::move(job), counter};
                RunJob(inlineJob);
                return;
            }

            state.queue.push_back(Job{std::move(job), counter});
        }

        state.queueCV.notify_one();
    }

    void ParallelFor(uint32_t count, uint32_t grainSize, const std::function<void(uint32_t begin, uint32_t end)> &fn) {
        if (count == 0)
            return;

        const uint32_t workers = WorkerCount();

        if (grainSize == 0) {
            // Auto: ~4 ranges per worker for load balance, but NEVER zero!!!
            const uint32_t targetRanges = (workers > 0) ? workers * 4 : 1;
            grainSize = (count + targetRanges -1 ) / targetRanges;
            if (grainSize == 0)
                grainSize = 1;
        }

        // Small workloads: not worth the queue traffic
        if (workers == 0 || count <= grainSize) {
            fn(0, count);
            return;
        }

        JobCounter counter;
        for (uint32_t begin = 0; begin < count; begin += grainSize) {
            const uint32_t end = (begin + grainSize < count) ? begin + grainSize : count;
            Execute([&fn, begin, end] { fn(begin, end); }, &counter);
        }

        Wait(counter);
    }

    void Wait(JobCounter& counter) {
        // Help execute while waiting
        // A full pool can't deadlock because the waiter is also a worker
        while (!counter.IsDone()) {
            if (!TryRunOneJob()) {
                // Queue empty but counter not done: something is still executing
                std::this_thread::yield();
            }
        }
    }
} // Jobs

} // Lutum