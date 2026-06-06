/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "segar/segar.h"
#include "integration_test/srv/PerfData.hpp"
#include "segar/task/lock_guard.h"
#include "perf_common.hpp"

using PerfData = integration_test::srv::PerfData;

namespace {
using rti::segar::service::Client;

constexpr uint32_t kTotalCalls = 1000;
const std::vector<size_t> kPayloadSizes = {64,    256,   1024,   4096,
                                           16384, 65536, 262144, 1048576};
// const std::vector<size_t> kPayloadSizes = {1048576};

// Max wait for one round so lost/timeout requests don't block forever.
constexpr std::chrono::seconds kRoundWaitTimeoutSeconds(300);

struct RoundState {
  std::mutex mutex;
  // std::condition_variable round_done_cv;
  std::shared_ptr<rti::segar::WaitEvent> round_done_cv =
      std::make_shared<rti::segar::WaitEvent>();
  std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> start_times;  // only for successfully sent requests
  std::vector<double> latencies_us;  // at most kTotalCalls entries per round
  uint64_t latency_count = 0;
  uint64_t total_latency_us = 0;
  uint32_t response_count = 0;
  uint32_t send_failed_count = 0;
  uint64_t round_id = 0;
  std::chrono::steady_clock::time_point round_start_time;
  bool round_complete = false;
};

void OnResponse(uint32_t seq,
                const std::shared_ptr<PerfData::Response>& response,
                RoundState* state,
                uint64_t expected_round_id) {
  if (!state) {
    return;
  }

  const auto end_time = std::chrono::steady_clock::now();
  double latency_ms = 0.0;
  uint32_t current_count = 0;
  bool should_notify_done = false;

  {
    ::rti::segar::LockGuard<std::mutex> lock(state->mutex);
    if (state->round_id != expected_round_id) {
      return;
    }
    auto it = state->start_times.find(seq);
    if (it == state->start_times.end()) {
      AWARN << "Orphan or duplicate response seq=" << seq
            << ", ignored (possible late response from previous round)";
      return;
    }

    const auto start_time = it->second;
    state->start_times.erase(it);
    if (response) {
      const auto latency_us =
          std::chrono::duration_cast<std::chrono::microseconds>(
              end_time - start_time)
              .count();
      const double lat_us = static_cast<double>(latency_us);
      latency_ms = lat_us / 1000.0;
      state->latencies_us.push_back(lat_us);
      state->total_latency_us += static_cast<uint64_t>(latency_us);
      ++state->latency_count;
      state->response_count++;
      current_count = state->response_count;
    } else {
      // Async callback gets nullptr on timeout; count it as failed.
      ++state->send_failed_count;
    }

    if (state->response_count + state->send_failed_count == kTotalCalls) {
      state->round_complete = true;
      should_notify_done = true;
    }
  }

  if (should_notify_done) {
    state->round_done_cv->Notify();
  }
}

}  // namespace

int main(int argc, char** argv) {
  rti::segar::Init(*argv);

  auto node = rti::segar::CreateNode("segar_perf_client_async");
  if (!node) {
    AERROR << "Failed to create node for service client.";
    return 1;
  }

  const std::string service_name = "perf_service";
  auto client = node->CreateClient<PerfData>(service_name);
  if (!client) {
    AERROR << "Failed to create service client.";
    return 1;
  }

  if (!client->WaitForService(std::chrono::seconds(5))) {
    AERROR << "Service not available";
    return 1;
  }

  rti::segar::service::RequestOptions opt;
  opt.timeout = std::chrono::milliseconds(2000);

  RoundState state;

  for (size_t size : kPayloadSizes) {
    uint64_t current_round_id;
    {
      ::rti::segar::LockGuard<std::mutex> lock(state.mutex);
      state.start_times.clear();
      state.latencies_us.clear();
      state.latencies_us.reserve(kTotalCalls);
      state.latency_count = 0;
      state.total_latency_us = 0;
      state.response_count = 0;
      state.send_failed_count = 0;
      state.round_id++;
      current_round_id = state.round_id;
      state.round_complete = false;
      state.round_start_time = std::chrono::steady_clock::now();
    }
    state.round_done_cv->Reset();

    AINFO << "Round start: payload_size=" << size
          << " kTotalCalls=" << kTotalCalls;

    for (uint32_t i = 0; i < kTotalCalls; ++i) {
      auto req = std::make_shared<PerfData::Request>();
      req->id(1);
      req->timestamp(std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count());
      req->sequence_number(i);
      req->data_size(static_cast<uint32_t>(size));

      std::vector<uint8_t> data(size);
      for (size_t j = 0; j < size; ++j) {
        data[j] = static_cast<uint8_t>(j & 0xFF);
      }
      req->data(std::move(data));

      const auto start_time = std::chrono::steady_clock::now();
      {
        ::rti::segar::LockGuard<std::mutex> lock(state.mutex);
        state.start_times[i] = start_time;
      }

      if (!client->AsyncSendRequest(
              req,
              [i, &state, current_round_id](
                  const std::shared_ptr<PerfData::Response>& resp) {
                OnResponse(i, resp, &state, current_round_id);
              },
              opt)) {
        AWARN << "AsyncSendRequest failed seq=" << i;
        ::rti::segar::LockGuard<std::mutex> lock(state.mutex);
        state.start_times.erase(i);
        ++state.send_failed_count;
        if (state.response_count + state.send_failed_count == kTotalCalls) {
          state.round_complete = true;
          state.round_done_cv->Notify();
        }
      }
    }

    // Must NOT hold state.mutex while waiting: the async callback (OnResponse)
    // runs on another thread and needs state.mutex to update state and call
    // Notify(). Holding the lock here would deadlock.
    const bool completed = state.round_done_cv->WaitFor(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            kRoundWaitTimeoutSeconds),
        [&state]() {
          std::lock_guard<std::mutex> lock(state.mutex);
          return state.round_complete;
        });

    perf::Stats round_stats;
    {
      ::rti::segar::LockGuard<std::mutex> lock(state.mutex);
      const auto total_end_time = std::chrono::steady_clock::now();
      const int64_t total_consumed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              total_end_time - state.round_start_time)
              .count();
      const double total_seconds =
          static_cast<double>(total_consumed_ms) / 1000.0;
      round_stats = perf::compute_stats(
          state.latencies_us, kTotalCalls, total_seconds,
          state.response_count, state.send_failed_count);

      AINFO << "Round end: payload_size=" << size
            << " total_time_ms=" << total_consumed_ms
            << " responses=" << state.response_count
            << " send_failures=" << state.send_failed_count
            << " completed=" << (completed ? "yes" : "timeout");
      if (!completed) {
        AERROR << "Round wait timeout after " << kRoundWaitTimeoutSeconds.count()
               << "s, some requests may be lost or still in flight.";
      }
    }

    AINFO << "latency(us): min=" << round_stats.min_us
          << " avg=" << round_stats.avg_us
          << " stddev=" << round_stats.stddev_us
          << " p10=" << round_stats.p10_us
          << " p30=" << round_stats.p30_us
          << " p50=" << round_stats.p50_us
          << " p90=" << round_stats.p90_us
          << " p99=" << round_stats.p99_us
          << " p99.9=" << round_stats.p999_us
          << " max=" << round_stats.max_us;
  }

  // All rounds done; trigger shutdown and wait for cleanup, then exit.
  rti::segar::OnShutdown(0);
  rti::segar::WaitForShutdown();
  return 0;
}
