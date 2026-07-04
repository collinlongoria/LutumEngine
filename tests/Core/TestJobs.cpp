/*
* File: TestJobs.cpp
* Project: LutumEngine
* Author: Collin
* Created on: 7/3/2026
*
* Copyright (c) 2026 Collin Longoria
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "Lutum/Core/Jobs.hpp"

struct JobsFixture {
    JobsFixture() {
        Lutum::Jobs::Initialize();
    }

    ~JobsFixture() {
        Lutum::Jobs::Shutdown();
    }
};

TEST_CASE_FIXTURE(JobsFixture, "ParallelFor computes correct sum") {
    std::atomic<uint64_t> sum{0};

    Lutum::Jobs::ParallelFor(1'000'000, 0, [&sum](uint32_t begin, uint32_t end) {
        uint64_t local = 0;

        for (uint32_t i = begin; i < end; ++i) {
            local += i;
        }

        sum += local;
    });

    CHECK(sum.load() == 499999500000ull);
}

TEST_CASE_FIXTURE(JobsFixture, "Nested wait does not deadlock") {
    Lutum::JobCounter outer;

    Lutum::Jobs::Execute([] {
        Lutum::Jobs::ParallelFor(100, 10, [](uint32_t, uint32_t) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }, &outer);

    Lutum::Jobs::Wait(outer);

    CHECK(true);
}