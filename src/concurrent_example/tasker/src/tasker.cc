/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "segar/segar.h"
#include "segar/task/task.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);
  AINFO << "=== Task Example: Demonstrating Concurrency Infrastructure ===";
  AINFO << "This example demonstrates all concurrency primitives:";
  AINFO << "  - Async: Execute tasks asynchronously and get results via future";
  AINFO << "  - Execute: Fire-and-forget task execution";
  AINFO << "  - TaskEvent: Event-based synchronization";
  AINFO << "  - LockGuard: Coroutine-safe mutex protection";
  AINFO << "  - Yield: Yield coroutine execution";
  AINFO << "  - SleepFor: Coroutine-safe sleep";
  AINFO << "";
  // Combined example: uses all concurrency primitives.
  AINFO << "--- Combined Example (showing all concurrency primitives) ---";
  {
    auto event1 = std::make_shared<rti::segar::TaskEvent>();
    auto event2 = std::make_shared<rti::segar::TaskEvent>();
    std::mutex mtx;
    std::vector<std::string> log;
    std::atomic<int> background_task_count{0};

    // Stage 1: prepare data (use Async to get a future)
    auto stage1 = [&mtx, &log, event1]() {
      AINFO << "[Combined] Stage 1: Preparing data...";
      rti::segar::SleepFor(std::chrono::milliseconds(50));
      {
        rti::segar::LockGuard<std::mutex> lock(
            mtx);  // Use LockGuard to protect shared data.
        log.push_back("Stage1: Data prepared");
      }
      event1->Notify();  // Notify via TaskEvent.
      AINFO << "[Combined] Stage 1: Completed";
    };

    // Stage 2: process data (wait for stage 1, use Async future)
    auto stage2 = [&mtx, &log, event1, event2]() {
      AINFO << "[Combined] Stage 2: Waiting for stage 1...";
      if (event1->Wait(
              std::chrono::milliseconds(200))) {  // Wait via TaskEvent.
        AINFO << "[Combined] Stage 2: Processing data...";
        rti::segar::SleepFor(std::chrono::milliseconds(50));
        {
          rti::segar::LockGuard<std::mutex> lock(
              mtx);  // Use LockGuard to protect shared data.
          log.push_back("Stage2: Data processed");
        }
        event2->Notify();  // Notify via TaskEvent.
        AINFO << "[Combined] Stage 2: Completed";
      }
    };

    // Stage 3: finalize (wait for stage 2, use Async future, show Yield)
    auto stage3 = [&mtx, &log, event2]() {
      AINFO << "[Combined] Stage 3: Waiting for stage 2...";
      if (event2->Wait(
              std::chrono::milliseconds(200))) {  // Wait via TaskEvent.
        AINFO << "[Combined] Stage 3: Finalizing with computation loop...";
        int sum = 0;
        constexpr int iterations = 100;
        for (int i = 0; i < iterations; ++i) {
          // Perform simple computation.
          sum += i * i;
          // Call yield per iteration to avoid starvation.
          rti::segar::Yield();  // Yield coroutine to avoid long monopolization.
        }
        {
          rti::segar::LockGuard<std::mutex> lock(
              mtx);  // Use LockGuard to protect shared data.
          log.push_back("Stage3: Finalized (sum=" + std::to_string(sum) + ")");
        }
        AINFO << "[Combined] Stage 3: Completed (computed sum: " << sum << ")";
      }
    };

    // Background task (Execute: fire-and-forget).
    auto background_task = [&mtx, &log, &background_task_count](int id) {
      AINFO << "[Combined] Background task " << id << " started";
      rti::segar::SleepFor(std::chrono::milliseconds(30));
      {
        rti::segar::LockGuard<std::mutex> lock(
            mtx);  // Use LockGuard to protect shared data.
        log.push_back("Background task " + std::to_string(id) + " completed");
      }
      background_task_count.fetch_add(1);
      AINFO << "[Combined] Background task " << id << " completed";
    };

    // Start main stages with Async and keep futures.
    auto f1 = rti::segar::Async(stage1);
    auto f2 = rti::segar::Async(stage2);
    auto f3 = rti::segar::Async(stage3);

    // Start background tasks with Execute (fire-and-forget).
    rti::segar::Execute(background_task, 1);
    rti::segar::Execute(background_task, 2);
    rti::segar::Execute(background_task, 3);

    // Wait for all Async tasks (via futures).
    if (f1.valid()) f1.wait();
    if (f2.valid()) f2.wait();
    if (f3.valid()) f3.wait();

    // Wait for background tasks to finish.
    rti::segar::SleepFor(std::chrono::milliseconds(100));

    // Print execution log.
    {
      rti::segar::LockGuard<std::mutex> lock(
          mtx);  // Use LockGuard to protect shared data.
      AINFO << "[Combined] Execution log (total entries: " << log.size()
            << "):";
      for (const auto& entry : log) {
        AINFO << "[Combined]   - " << entry;
      }
    }
    AINFO << "[Combined] Background tasks completed: "
          << background_task_count.load();

    AINFO << "[Combined] Summary: Used Async (with future), Execute "
             "(fire-and-forget), "
          << "TaskEvent (synchronization), LockGuard (mutex), Yield, and "
             "SleepFor";
  }

  AINFO << "\n=== Task Example Completed ===";
  AINFO << "Waiting for shutdown...";
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
