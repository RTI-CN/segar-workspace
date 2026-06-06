/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "sync_integration_node.h"

#include "segar/time/time.h"

using rti::segar::Time;

bool SyncIntegrationNode::Init() {
  AINFO << "SyncIntegrationNode initialized";
  return true;
}

bool SyncIntegrationNode::Proc(
    const std::shared_ptr<IntegrationTestData>& msg0,
    const std::shared_ptr<IntegrationTestData>& msg1,
    const std::shared_ptr<IntegrationTestData>& msg2,
    const std::shared_ptr<IntegrationTestData>& msg3) {
  if (!msg0 && !msg1 && !msg2 && !msg3) {
    AERROR << "SyncIntegrationNode::Proc called with all null messages";
    return false;
  }

  // Choose base timestamp (prefer first input; fall back if needed).
  uint64_t base_ts_ns = 0;
  if (msg0) {
    base_ts_ns = msg0->timestamp();
  } else if (msg1) {
    base_ts_ns = msg1->timestamp();
  } else if (msg2) {
    base_ts_ns = msg2->timestamp();
  } else if (msg3) {
    base_ts_ns = msg3->timestamp();
  }

  if (base_ts_ns == 0) {
    AERROR << "SyncIntegrationNode::Proc invalid base timestamp (0)";
    return false;
  }

  UpdateMaxDiff(base_ts_ns, msg0);
  UpdateMaxDiff(base_ts_ns, msg1);
  UpdateMaxDiff(base_ts_ns, msg2);
  UpdateMaxDiff(base_ts_ns, msg3);

  // Update Proc time window for frequency calculation.
  const uint64_t now_ns = Time::Now().ToNanosecond();
  if (first_proc_ts_ns_ == 0 || now_ns < first_proc_ts_ns_) {
    first_proc_ts_ns_ = now_ns;
  }
  if (now_ns > last_proc_ts_ns_) {
    last_proc_ts_ns_ = now_ns;
  }

  // Count how many times each input topic actually participates in fused frames.
  if (msg0) {
    ++input0_count_;
  }
  if (msg1) {
    ++input1_count_;
  }
  if (msg2) {
    ++input2_count_;
  }
  if (msg3) {
    ++input3_count_;
  }

  // Mark that at least one fused frame has been processed.
  has_first_proc_.store(true, std::memory_order_relaxed);

  return true;
}

void SyncIntegrationNode::UpdateMaxDiff(
    uint64_t base_ts_ns,
    const std::shared_ptr<IntegrationTestData>& msg) {
  if (!msg) {
    return;
  }
  const uint64_t ts = msg->timestamp();
  if (ts == 0) {
    return;
  }
  uint64_t diff_ns = (ts > base_ts_ns) ? (ts - base_ts_ns) : (base_ts_ns - ts);
  uint64_t diff_ms = diff_ns / 1000000;
  if (diff_ms > max_abs_diff_ms_) {
    max_abs_diff_ms_ = diff_ms;
  }
}

void SyncIntegrationNode::TimeoutProc() {
  // Only start reporting timeout after the first fused frame has been processed.
  if (!has_first_proc_.load(std::memory_order_relaxed)) {
    return;
  }

  AERROR << "ITEST_SYNC_TIMEOUT node=SyncIntegrationNode";
}

uint64_t SyncIntegrationNode::GetTimeStamp(
    size_t index,
    const std::shared_ptr<IntegrationTestData>& msg0,
    const std::shared_ptr<IntegrationTestData>& msg1,
    const std::shared_ptr<IntegrationTestData>& msg2,
    const std::shared_ptr<IntegrationTestData>& msg3) {
  switch (index) {
    case 0:
      if (msg0) {
        return msg0->timestamp();
      }
      break;
    case 1:
      if (msg1) {
        return msg1->timestamp();
      }
      break;
    case 2:
      if (msg2) {
        return msg2->timestamp();
      }
      break;
    case 3:
      if (msg3) {
        return msg3->timestamp();
      }
      break;
    default:
      break;
  }

  AERROR << "SyncIntegrationNode::GetTimeStamp invalid index: " << index;
  return 0;
}

void SyncIntegrationNode::Shutdown() {
  // Query internal sync statistics from SyncComponent.
  auto stats = GetStatisticsInfo();

  AINFO << "ITEST_SYNC_STAT node=SyncIntegrationNode"
        << " normal=" << stats.normal_num_
        << " latency=" << stats.latency_num_
        << " timeout=" << stats.timeout_num_
        << " max_abs_diff_ms=" << max_abs_diff_ms_;

  // Calculate per-topic frequencies based on Proc time window.
  double duration_sec = 0.0;
  if (first_proc_ts_ns_ != 0 && last_proc_ts_ns_ > first_proc_ts_ns_) {
    duration_sec =
        static_cast<double>(last_proc_ts_ns_ - first_proc_ts_ns_) / 1e9;
  }

  auto calc_freq = [duration_sec](uint64_t count) -> double {
    if (duration_sec <= 0.0) {
      return 0.0;
    }
    return static_cast<double>(count) / duration_sec;
  };

  AINFO << "ITEST_SYNC_TOPIC_STAT node=SyncIntegrationNode"
        << " input=sensor_topic_1"
        << " count=" << input0_count_
        << " freq_hz=" << calc_freq(input0_count_);

  AINFO << "ITEST_SYNC_TOPIC_STAT node=SyncIntegrationNode"
        << " input=sensor_topic_2"
        << " count=" << input1_count_
        << " freq_hz=" << calc_freq(input1_count_);

  AINFO << "ITEST_SYNC_TOPIC_STAT node=SyncIntegrationNode"
        << " input=sensor_topic_3"
        << " count=" << input2_count_
        << " freq_hz=" << calc_freq(input2_count_);

  AINFO << "ITEST_SYNC_TOPIC_STAT node=SyncIntegrationNode"
        << " input=sensor_topic_4"
        << " count=" << input3_count_
        << " freq_hz=" << calc_freq(input3_count_);

  SyncComponent<IntegrationTestData, IntegrationTestData,
                IntegrationTestData,
                IntegrationTestData>::Shutdown();
}

