/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "integration_test/action/PerfAction.hpp"

#include "segar/segar.h"
#include "segar/task/lock_guard.h"

namespace {
using rti::segar::action::ActionClient;
using rti::segar::action::ActionOptions;
using rti::segar::action::GoalID;
using rti::segar::action::GoalStatusCode;
using PerfAction = integration_test::action::PerfAction;

static const std::vector<size_t> kPayloadSizes = {64, 1024 * 1024};
static const std::vector<int> kIters = {200, 2000};

struct LatStatMs {
  double avg = 0.0;
  double p50 = 0.0;
  double p90 = 0.0;
  double p99 = 0.0;
  double max = 0.0;
};

LatStatMs ComputeStatsMs(std::vector<uint64_t> lat_us) {
  LatStatMs s;
  if (lat_us.empty()) {
    return s;
  }

  uint64_t sum = 0;
  uint64_t max_v = 0;
  for (auto v : lat_us) {
    sum += v;
    if (v > max_v) {
      max_v = v;
    }
  }

  std::sort(lat_us.begin(), lat_us.end());
  auto pick = [&](double p) {
    size_t idx = static_cast<size_t>(p * (lat_us.size() - 1));
    return lat_us[idx] / 1000.0;
  };

  s.avg =
      static_cast<double>(sum) / static_cast<double>(lat_us.size()) / 1000.0;
  s.p50 = pick(0.50);
  s.p90 = pick(0.90);
  s.p99 = pick(0.99);
  s.max = max_v / 1000.0;
  return s;
}

struct AsyncTracker {
  std::mutex mutex;
  std::shared_ptr<rti::segar::WaitEvent> done_event =
      std::make_shared<rti::segar::WaitEvent>();
  std::unordered_map<std::string, std::chrono::steady_clock::time_point>
      start_times;
  std::unordered_set<std::string> pending_goals;
  std::vector<uint64_t> lat_us;
  uint64_t run_id = 0;
  uint64_t attempt = 0;
  uint64_t sent = 0;
  uint64_t completed = 0;
  uint64_t success = 0;
  uint64_t expected = 0;
  uint64_t stale_results = 0;  // result for unknown/previous-run goal_id
  bool running = false;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point end_time;

  void StartRun(int iter) {
    // Clear stale notifications from the previous run.
    done_event->Reset();
    ::rti::segar::LockGuard<std::mutex> lock(mutex);
    ++run_id;
    attempt = static_cast<uint64_t>(iter);
    sent = 0;
    completed = 0;
    success = 0;
    expected = 0;
    stale_results = 0;
    running = true;
    start_times.clear();
    pending_goals.clear();
    lat_us.clear();
    lat_us.reserve(iter);
    start_time = std::chrono::steady_clock::now();
  }

  void MarkSend(const GoalID& goal_id) {
    ::rti::segar::LockGuard<std::mutex> lock(mutex);
    const auto key = rti::segar::action::internal::GoalIDToString(goal_id);
    start_times[key] = std::chrono::steady_clock::now();
    pending_goals.insert(key);
    ++sent;  // goals successfully sent
  }

  void FinishSending() {
    ::rti::segar::LockGuard<std::mutex> lock(mutex);
    expected = sent;
    // Some results may arrive before we finish sending; use completed here.
    if (expected == 0 || completed >= expected) {
      end_time = std::chrono::steady_clock::now();
      running = false;
      done_event->Notify();
    }
  }

  void OnResult(const GoalID& goal_id, GoalStatusCode status) {
    ::rti::segar::LockGuard<std::mutex> lock(mutex);
    const auto key = rti::segar::action::internal::GoalIDToString(goal_id);
    // Filter out late/duplicate/unknown results so we never count more than
    // sent.
    auto pending_it = pending_goals.find(key);
    if (pending_it == pending_goals.end()) {
      ++stale_results;
      return;
    }
    pending_goals.erase(pending_it);
    ++completed;

    auto it = start_times.find(key);
    if (it != start_times.end()) {
      const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::steady_clock::now() - it->second)
                          .count();
      if (us > 0 && status == GoalStatusCode::STATUS_SUCCEEDED) {
        lat_us.push_back(static_cast<uint64_t>(us));
      }
      start_times.erase(it);
    }
    if (status == GoalStatusCode::STATUS_SUCCEEDED) {
      ++success;
    }

    if (running && expected > 0 && completed >= expected) {
      end_time = std::chrono::steady_clock::now();
      running = false;
      done_event->Notify();
      return;
    }
  }
};

void RunAsyncTest(const std::shared_ptr<ActionClient<PerfAction>>& client,
                  size_t payload_size,
                  int iter, AsyncTracker* tracker) {
  tracker->StartRun(iter);

  std::vector<uint8_t> payload(payload_size, 0x5a);

  // Send all goals asynchronously
  for (int i = 0; i < iter; ++i) {
    PerfAction::Goal goal;
    goal.seq(static_cast<uint32_t>(i));
    goal.payload_size(static_cast<uint32_t>(payload_size));
    goal.payload(payload);
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    GoalID goal_id;
    if (!client || !client->AsyncSendGoal(goal, &goal_id)) {
      continue;
    }

    tracker->MarkSend(goal_id);
  }

  tracker->FinishSending();
  const bool done = tracker->done_event->WaitFor(
      std::chrono::seconds(60), [tracker]() {
        ::rti::segar::LockGuard<std::mutex> lock(tracker->mutex);
        return !tracker->running;
      });
  if (!done) {
    ::rti::segar::LockGuard<std::mutex> lock(tracker->mutex);
    tracker->running = false;
    tracker->end_time = std::chrono::steady_clock::now();
  }

  LatStatMs stats;
  double elapsed_s = 0.0;
  uint64_t sent = 0;
  uint64_t completed = 0;
  uint64_t success = 0;
  uint64_t stale_results = 0;
  {
    ::rti::segar::LockGuard<std::mutex> lock(tracker->mutex);
    stats = ComputeStatsMs(tracker->lat_us);
    sent = tracker->sent;
    completed = tracker->completed;
    success = tracker->success;
    stale_results = tracker->stale_results;
    elapsed_s = std::chrono::duration_cast<std::chrono::duration<double>>(
                    tracker->end_time - tracker->start_time)
                    .count();
  }

  const uint64_t missing = (sent > success) ? (sent - success) : 0;
  const double loss =
      sent > 0 ? static_cast<double>(missing) / static_cast<double>(sent) : 0.0;
  const double throughput_MBps =
      elapsed_s > 0.0 ? (static_cast<double>(payload_size) * 2.0 * success) /
                            (1024.0 * 1024.0) / elapsed_s
                      : 0.0;

  AINFO << "[SEGAR][ASYNC] payload=" << payload_size << "B iter=" << iter
        << " sent=" << sent << " completed=" << completed
        << " success=" << success << " stale=" << stale_results
        << " loss=" << loss << " thr=" << throughput_MBps << " MB/s"
        << " avg=" << stats.avg << " ms"
        << " p50=" << stats.p50 << " ms"
        << " p90=" << stats.p90 << " ms"
        << " p99=" << stats.p99 << " ms"
        << " max=" << stats.max << " ms";
}
}  // namespace

int main(int argc, char** argv) {
  rti::segar::Init(*argv);

  auto node = rti::segar::CreateNode("perf_action_client_async");
  if (!node) {
    AERROR << "Failed to create node for action client.";
    return 1;
  }

  auto tracker = std::make_shared<AsyncTracker>();
  ActionClient<PerfAction>::GoalCallbacks callbacks;
  callbacks.on_result = [weak = std::weak_ptr<AsyncTracker>(tracker)](
                            ActionClient<PerfAction>&, const GoalID& goal_id,
                            const PerfAction::Result&, GoalStatusCode status) {
    if (auto locked = weak.lock()) {
      locked->OnResult(goal_id, status);
    }
  };

  ActionOptions option;
  // If GetResult is backpressured (e.g. server overload), avoid spamming
  // timeouts.
  option.wait_result_timeout_ms = 1000.0;
  option.rpc_timeout_ms = 1000;
  option.max_client_active_goals = 4096;
  option.max_client_concurrent_goals = 16;

  auto client =
      node->CreateActionClient<PerfAction>("perf_action", callbacks, option);
  if (!client) {
    AERROR << "Failed to create action client.";
    return 1;
  }

  rti::segar::Async([client, tracker]() {
    for (size_t payload_size : kPayloadSizes) {
      for (int iter : kIters) {
        RunAsyncTest(client, payload_size, iter, tracker.get());
      }
    }
    rti::segar::AsyncShutdown();
  });

  return 0;
}
