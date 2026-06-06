/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include "integration_test/msg/IntegrationTestData.hpp"

#include "segar/segar.h"

using integration_test::msg::IntegrationTestData;
using namespace std::chrono;

static constexpr int kIters = 10000;
static constexpr uint64_t kFirstReplyRetryNs = 200ULL * 1000 * 1000;

// 你的测试数据量
static const std::vector<size_t> kPayloadSizes = {
    64, 256, 1024, 4 * 1024, 16 * 1024, 64 * 1024, 256 * 1024, 1024 * 1024};

static inline uint64_t NowNs() {
  return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch())
      .count();
}

struct LatStatUs {
  double avg = 0, p10 = 0, p30 = 0, p50 = 0, p90 = 0, p99 = 0, max = 0;
};

static LatStatUs ComputeStatsUs(std::vector<uint64_t> ns_vec) {
  LatStatUs s;
  if (ns_vec.empty()) return s;

  uint64_t sum = 0;
  uint64_t mx = 0;
  for (auto v : ns_vec) {
    sum += v;
    if (v > mx) mx = v;
  }

  std::sort(ns_vec.begin(), ns_vec.end());
  auto pick = [&](double p) -> double {
    size_t idx = static_cast<size_t>(p * (ns_vec.size() - 1));
    return ns_vec[idx] / 1000.0;
  };

  s.avg = (double)sum / (double)ns_vec.size() / 1000.0;
  s.p10 = pick(0.10);
  s.p30 = pick(0.30);
  s.p50 = pick(0.50);
  s.p90 = pick(0.90);
  s.p99 = pick(0.99);
  s.max = mx / 1000.0;
  return s;
}

class SegarPingPongA {
 public:
  explicit SegarPingPongA(const std::shared_ptr<rti::segar::Node>& node)
      : node_(node) {
    // Writer: A -> B

    writer_ = node_->CreateWriter<IntegrationTestData>("perf/a_to_b");

    // Reader: B -> A
    rti::segar::ReaderConfig rcfg;
    rcfg.topic_name = "perf/b_to_a";
    rcfg.pending_queue_size = 64;

    auto cb = [this](const std::shared_ptr<IntegrationTestData>& msg) {
      if (await_first_reply_.load(std::memory_order_acquire)) {
        await_first_reply_.store(false, std::memory_order_release);
      }

      uint64_t rtt = NowNs() - last_send_ns_.load(std::memory_order_acquire);
      lat_.push_back(rtt);

      ++seq_;
      if (seq_ >= kIters) {
        ReportOnePayload();

        if (payload_idx_ + 1 >= kPayloadSizes.size()) {
          std::printf("All payload tests done.\n");

          std::fflush(stdout);

          StopRetryThread();
          rti::segar::OnShutdown(0);
          return;
        }
        StartPayload(payload_idx_ + 1);
        return;
      }
      SendNext();
    };
    reader_ = node_->CreateReader<IntegrationTestData>(rcfg, cb);

    // 预分配：避免频繁扩容影响统计（尤其大包）
    lat_.reserve(kIters);

    StartPayload(0);
    StartFirstReplyRetry();
  }

  ~SegarPingPongA() { StopRetryThread(); }

 private:
  void StopRetryThread() {
    retry_running_.store(false, std::memory_order_release);
    if (retry_thread_.joinable()) {
      retry_thread_.join();
    }
  }

  void StartFirstReplyRetry() {
    retry_thread_ = std::thread([this]() {
      while (retry_running_.load(std::memory_order_acquire)) {
        if (await_first_reply_.load(std::memory_order_acquire)) {
          auto now = NowNs();
          auto last_send_ns = last_send_ns_.load(std::memory_order_acquire);
          if (last_send_ns != 0 && now - last_send_ns > kFirstReplyRetryNs) {
            std::shared_ptr<IntegrationTestData> msg_to_retry;
            {
              std::lock_guard<std::mutex> lock(send_mutex_);
              last_send_ns = last_send_ns_.load(std::memory_order_relaxed);
              if (last_send_ns != 0 && now - last_send_ns > kFirstReplyRetryNs) {
                msg_to_retry = msg_;
                last_send_ns_.store(now, std::memory_order_release);
              }
            }
            if (msg_to_retry != nullptr) {
              writer_->Write(msg_to_retry);
            }
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
    });
  }

  void StartPayload(size_t idx) {
    payload_idx_ = idx;
    payload_ = kPayloadSizes.at(payload_idx_);
    seq_ = 0;
    lat_.clear();

    {
      std::lock_guard<std::mutex> lock(send_mutex_);
      msg_ = std::make_shared<IntegrationTestData>();
      msg_->sensor_id(1);
      msg_->data_size(static_cast<uint32_t>(payload_));
      msg_->data().resize(payload_);
      last_send_ns_.store(0, std::memory_order_release);
    }

    test_start_ns_ = NowNs();

    await_first_reply_.store(true, std::memory_order_release);
    SendNext();
  }

  void SendNext() {
    auto now = NowNs();
    std::shared_ptr<IntegrationTestData> msg_to_send;
    {
      std::lock_guard<std::mutex> lock(send_mutex_);
      msg_->sequence_number(seq_);
      msg_->timestamp(now);
      msg_to_send = msg_;
      last_send_ns_.store(now, std::memory_order_release);
    }
    writer_->Write(msg_to_send);
  }

  void ReportOnePayload() {
    double elapsed_s = (NowNs() - test_start_ns_) / 1e9;
    auto st = ComputeStatsUs(lat_);
    double tps = (double)kIters / elapsed_s;

    // 吞吐定义：按你现在模型（收到回复再发下一条），常用 payload *
    // TPS（单向有效吞吐）
    double thr_MBps = (payload_ * tps) / (1024.0 * 1024.0);

    std::printf(
        "[Segar] payload=%zuB iters=%d elapsed=%.3fs TPS=%.2f Throughput=%.2f "
        "MB/s | "
        "avg=%.2fus p10=%.2fus p30=%.2fus p50=%.2fus p90=%.2fus p99=%.2fus "
        "max=%.2fus\n",
        payload_, kIters, elapsed_s, tps, thr_MBps, st.avg, st.p10, st.p30,
        st.p50, st.p90, st.p99, st.max);
    std::fflush(stdout);
  }

 private:
  std::shared_ptr<rti::segar::Node> node_;
  std::shared_ptr<rti::segar::Writer<IntegrationTestData>> writer_;
  std::shared_ptr<rti::segar::Reader<IntegrationTestData>> reader_;

  size_t payload_idx_ = 0;
  size_t payload_ = 0;

  int seq_ = 0;
  uint64_t test_start_ns_ = 0;
  std::atomic<uint64_t> last_send_ns_{0};

  std::shared_ptr<IntegrationTestData> msg_;
  std::mutex send_mutex_;
  std::thread retry_thread_;
  std::atomic<bool> await_first_reply_{true};
  std::atomic<bool> retry_running_{true};
  std::vector<uint64_t> lat_;
};

int main(int argc, char* argv[]) {
  rti::segar::Init(argv[0]);

  auto node = rti::segar::CreateNode("segar_perf_node_a");
  SegarPingPongA runner(node);

  rti::segar::WaitForShutdown();
  return 0;
}
