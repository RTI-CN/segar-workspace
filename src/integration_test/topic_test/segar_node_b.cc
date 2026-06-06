/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>
#include <string>

#include "segar/segar.h"
#include "integration_test/msg/IntegrationTestData.hpp"

using integration_test::msg::IntegrationTestData;

int main(int argc, char* argv[]) {
  rti::segar::Init(argv[0]);

  // Node B
  auto node = rti::segar::CreateNode("segar_perf_node_b");

  // Reader: A -> B
  rti::segar::ReaderConfig reader_cfg;
  reader_cfg.topic_name = "perf/a_to_b";
  reader_cfg.pending_queue_size = 64;


  auto writer = node->CreateWriter<IntegrationTestData>("perf/b_to_a");

  auto cb = [writer](const std::shared_ptr<IntegrationTestData>& msg) {
    // 原样回包（不做业务处理）
    // 注意：如果 writer 只能接受 shared_ptr，就直接转发；
    // 如果 writer 需要 const IntegrationTestData&，则改成 *msg。
    writer->Write(std::move(msg));
  };

  auto reader = node->CreateReader<IntegrationTestData>(reader_cfg, cb);

  rti::segar::WaitForShutdown();
  return 0;
}
