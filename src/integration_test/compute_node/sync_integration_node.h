/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_SYNC_INTEGRATION_NODE_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_SYNC_INTEGRATION_NODE_H

#include <memory>
#include <atomic>

#include "segar/class_loader/class_loader.h"
#include "segar/component/sync_component.h"
#include "integration_test/msg/IntegrationTestData.hpp"

using rti::segar::SyncComponent;
using integration_test::msg::IntegrationTestData;

// SyncIntegrationNode:
// - Uses SyncComponent to time-synchronize multiple IntegrationTestData streams.
// - Aggregates basic statistics and emits summary logs for integration tests.
class SyncIntegrationNode
    : public SyncComponent<IntegrationTestData, IntegrationTestData,
                           IntegrationTestData, IntegrationTestData> {
 public:
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& msg0,
            const std::shared_ptr<IntegrationTestData>& msg1,
            const std::shared_ptr<IntegrationTestData>& msg2,
            const std::shared_ptr<IntegrationTestData>& msg3) override;

  void TimeoutProc() override;

  uint64_t GetTimeStamp(size_t index,
                        const std::shared_ptr<IntegrationTestData>& msg0,
                        const std::shared_ptr<IntegrationTestData>& msg1,
                        const std::shared_ptr<IntegrationTestData>& msg2,
                        const std::shared_ptr<IntegrationTestData>& msg3) override;
  void Shutdown() override;

 private:
  // Aggregated maximum absolute time difference (ms) among synchronized inputs.
  uint64_t max_abs_diff_ms_ = 0;

  // Proc time window (for frequency calculation).
  uint64_t first_proc_ts_ns_ = 0;
  uint64_t last_proc_ts_ns_ = 0;

  // Per-input processed counts (only when the message exists in a fused frame).
  uint64_t input0_count_ = 0;  // sensor_topic_1 (REQUIRED)
  uint64_t input1_count_ = 0;  // sensor_topic_2 (REQUIRED)
  uint64_t input2_count_ = 0;  // sensor_topic_3 (WAITABLE)
  uint64_t input3_count_ = 0;  // sensor_topic_4 (OPTIONAL)

  // Whether at least one fused frame has been processed.
  std::atomic<bool> has_first_proc_{false};

  // Update max_abs_diff_ms_ using a base timestamp and a single message.
  void UpdateMaxDiff(uint64_t base_ts_ns,
                     const std::shared_ptr<IntegrationTestData>& msg);
};

SEGAR_REGISTER_COMPONENT(SyncIntegrationNode)

#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_SYNC_INTEGRATION_NODE_H

