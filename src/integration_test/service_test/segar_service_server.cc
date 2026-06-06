/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>
#include <string>
#include <vector>

#include "segar/segar.h"

// TODO: 替换为你实际生成的 IDL 头文件
#include "integration_test/srv/PerfData.hpp"

using PerfData = integration_test::srv::PerfData;

int main(int argc, char* argv[]) {
  rti::segar::Init(argv[0]);

  std::string service_name = "perf_service";  // 对齐 ROS2
  if (argc > 1) {
    service_name = argv[1];  // 允许自定义服务名
  }

  auto server_node = rti::segar::CreateNode("segar_perf_service_server");

  auto server = server_node->CreateService<PerfData>(
      service_name,
      [](const std::shared_ptr<PerfData::Request>& request,
         std::shared_ptr<PerfData::Response>& response) {
        // Echo：原封不动返回
        // TODO: 按你的 IDL 字段改名
        response->id(request->id());
        response->timestamp(request->timestamp());
        response->sequence_number(request->sequence_number());
        response->data_size(request->data_size());
        response->data(std::move(request->data()));  // 假设是 std::vector<uint8_t>
      });

  AINFO << "Segar service server started: " << service_name;

  rti::segar::WaitForShutdown();
  return 0;
}
