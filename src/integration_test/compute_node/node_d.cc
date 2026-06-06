/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_d.h"

#include <chrono>

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeDComponent::Init() {
  // 创建发布者
  node_d_writer_ = node_->CreateWriter<IntegrationTestData>("NodeD");
  add_two_client_ = node_->CreateClient<AddTwoInts>("add_two_ints");

  // 初始化序列号
  sequence_num_ = 0;

  AINFO << "NodeDComponent initialized";
  return true;
}

bool NodeDComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_c1_msg,
    const std::shared_ptr<IntegrationTestData>& node_c2_msg,
    const std::shared_ptr<IntegrationTestData>& node_c3_msg,
    const std::shared_ptr<IntegrationTestData>& node_c4_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_c1_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeD input=NodeC1 seq="
        << node_c1_msg->sequence_number() << " latency_ns=" << latency_ns;

  if (add_two_client_) {
    ++add_two_seq_;
    if (add_two_seq_ % 10 == 0) {
      auto request = std::make_shared<AddTwoInts::Request>();
      request->a(static_cast<int64_t>(add_two_seq_));
      request->b(static_cast<int64_t>(add_two_seq_ + 10));

      rti::segar::service::RequestOptions options;
      options.timeout = std::chrono::milliseconds(200);
      auto res = add_two_client_->SyncSendRequest(request, options);
      if (res) {
        AINFO << "NodeD sync add_two_ints response=" << res->sum();
      } else {
        AINFO << "NodeD sync add_two_ints timeout or not ready";
      }

      auto async_request = std::make_shared<AddTwoInts::Request>();
      async_request->a(static_cast<int64_t>(add_two_seq_));
      async_request->b(static_cast<int64_t>(add_two_seq_ + 100));
      rti::segar::service::Client<AddTwoInts>::ResponseCallback callback =
          [](const std::shared_ptr<AddTwoInts::Response>& response) {
            if (response) {
              AINFO << "NodeD async add_two_ints response=" << response->sum();
            } else {
              AINFO << "NodeD async add_two_ints timeout or not ready";
            }
          };
      rti::segar::service::Client<AddTwoInts>::RequestHandle handle;
      add_two_client_->AsyncSendRequest(async_request, callback, {}, &handle);
    }
  }

  // 处理接收的消息（此处仅为示例）
  AINFO << "NodeD received messages :::" << node_c1_msg.get()->sequence_number()
        << ", " << node_c2_msg.get()->sequence_number() << ", "
        << node_c3_msg.get()->sequence_number() << ", "
        << node_c4_msg.get()->sequence_number() << std::endl;
  cpu_burner_ms(25);

  // 创建并发布NodeD消息
  auto node_d_msg = std::make_shared<IntegrationTestData>();
  node_d_msg->sensor_id(0);  // NodeD的sensor_id设为0
  node_d_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_d_msg->sequence_number(sequence_num_++);
  node_d_msg->data_size(64 * 1024);  // 64KB数据
  node_d_msg->data().resize(64 * 1024);

  // 填充64KB数据
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_d_msg->data()[i] = static_cast<unsigned char>('B');
  }

  node_d_writer_->Write(node_d_msg);

  AINFO << "NodeD published message #" << sequence_num_;

  return true;
}
