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
#include <vector>

#include "integration_test/action/PerfAction.hpp"

#include "segar/segar.h"

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
  double p10 = 0.0;
  double p30 = 0.0;
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
  s.p10 = pick(0.10);
  s.p30 = pick(0.30);
  s.p50 = pick(0.50);
  s.p90 = pick(0.90);
  s.p99 = pick(0.99);
  s.max = max_v / 1000.0;
  return s;
}

void RunSyncTest(const std::shared_ptr<ActionClient<PerfAction>>& client,
                 size_t payload_size,
                 int iter) {
  std::vector<uint8_t> payload(payload_size, 0x5a);
  std::vector<uint64_t> lat_us;
  lat_us.reserve(iter);

  int sent = 0;
  int success = 0;

  const auto test_start = std::chrono::steady_clock::now();

  for (int i = 0; i < iter; ++i) {
    PerfAction::Goal goal;
    goal.seq(static_cast<uint32_t>(i));
    goal.payload_size(static_cast<uint32_t>(payload_size));
    goal.payload(payload);

    GoalID goal_id;
    const auto t0 = std::chrono::steady_clock::now();
    if (!client || !client->SyncSendGoal(goal, &goal_id)) {
      continue;
    }
    ++sent;

    PerfAction::Result result;
    GoalStatusCode status = GoalStatusCode::STATUS_UNKNOWN;
    if (!client->WaitForResult(goal_id, &result, &status)) {
      continue;
    }

    if (status != GoalStatusCode::STATUS_SUCCEEDED) {
      continue;
    }

    const auto t1 = std::chrono::steady_clock::now();
    const auto us =
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    if (us > 0) {
      lat_us.push_back(static_cast<uint64_t>(us));
    }
    ++success;
  }

  const auto test_end = std::chrono::steady_clock::now();
  const double elapsed_s =
      std::chrono::duration_cast<std::chrono::duration<double>>(test_end -
                                                                test_start)
          .count();

  const double loss =
      sent > 0 ? static_cast<double>(sent - success) / static_cast<double>(sent)
               : 0.0;
  const double throughput_MBps =
      elapsed_s > 0.0 ? (static_cast<double>(payload_size) * 2.0 * success) /
                            (1024.0 * 1024.0) / elapsed_s
                      : 0.0;

  const auto stats = ComputeStatsMs(lat_us);

  AINFO << "[SEGAR][SYNC] payload=" << payload_size << "B iter=" << iter
        << " sent=" << sent << " success=" << success << " loss=" << loss
        << " elapsed=" << elapsed_s << " s"
        << " thr=" << throughput_MBps << " MB/s"
        << " avg=" << stats.avg << " ms"
        << " p10=" << stats.p10 << " ms"
        << " p30=" << stats.p30 << " ms"
        << " p50=" << stats.p50 << " ms"
        << " p90=" << stats.p90 << " ms"
        << " p99=" << stats.p99 << " ms"
        << " max=" << stats.max << " ms";
}
}  // namespace

int main(int argc, char** argv) {
  rti::segar::Init(*argv);

  auto node = rti::segar::CreateNode("perf_action_client_sync");
  if (!node) {
    AERROR << "Failed to create node for action client.";
    return 1;
  }

  ActionClient<PerfAction>::GoalCallbacks callbacks;
  ActionOptions option;
  option.wait_result_timeout_ms = 1000.0;
  option.rpc_timeout_ms = 1000;
  option.max_client_active_goals = 4096;
  option.max_client_concurrent_goals = 256;

  auto client =
      node->CreateActionClient<PerfAction>("perf_action", callbacks, option);
  if (!client) {
    AERROR << "Failed to create action client.";
    return 1;
  }
  if (!client->WaitForService(std::chrono::seconds(3))) {
    AWARN << "Action service not ready after 3s, continuing anyway.";
  }

  rti::segar::Async([client]() {
    for (size_t payload_size : kPayloadSizes) {
      for (int iter : kIters) {
        RunSyncTest(client, payload_size, iter);
      }
    }
    rti::segar::AsyncShutdown();
  });

  rti::segar::WaitForShutdown();
  return 0;
}
