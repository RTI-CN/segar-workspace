/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_b.h"

#include <chrono>

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"
bool NodeBComponent::Init() {
  // 创建发布者
  node_b_writer_ = node_->CreateWriter<IntegrationTestData>("NodeB");
  add_two_client_ = node_->CreateClient<AddTwoInts>("add_two_ints");

  // 初始化序列号
  sequence_num_ = 0;

  AINFO << "NodeBComponent initialized";
  return true;
}

bool NodeBComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& sensor2_msg,
    const std::shared_ptr<IntegrationTestData>& sensor3_msg,
    const std::shared_ptr<IntegrationTestData>& sensor6_msg,
    const std::shared_ptr<IntegrationTestData>& sensor8_msg,
    const std::shared_ptr<IntegrationTestData>& sensor9_msg,
    const std::shared_ptr<IntegrationTestData>& node_a_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - sensor2_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeB input=sensor_topic_2 seq="
        << sensor2_msg->sequence_number() << " latency_ns=" << latency_ns;

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
        AINFO << "NodeB sync add_two_ints response=" << res->sum();
      } else {
        AINFO << "NodeB sync add_two_ints timeout or not ready";
      }

      auto async_request = std::make_shared<AddTwoInts::Request>();
      async_request->a(static_cast<int64_t>(add_two_seq_));
      async_request->b(static_cast<int64_t>(add_two_seq_ + 100));
      rti::segar::service::Client<AddTwoInts>::ResponseCallback callback =
          [](const std::shared_ptr<AddTwoInts::Response>& response) {
            if (response) {
              AINFO << "NodeB async add_two_ints response=" << response->sum();
            } else {
              AINFO << "NodeB async add_two_ints timeout or not ready";
            }
          };
      rti::segar::service::Client<AddTwoInts>::RequestHandle handle;
      add_two_client_->AsyncSendRequest(async_request, callback, {}, &handle);
    }
  }
  // cpu_burner_ms(1);

  // 处理接收的消息（此处仅为示例）
  AINFO << "NodeB received messages :::" << sensor2_msg.get()->sequence_number()
        << ", " << sensor3_msg.get()->sequence_number() << ", "
        << sensor6_msg.get()->sequence_number() << ", "
        << sensor8_msg.get()->sequence_number() << ", "
        << sensor9_msg.get()->sequence_number() << ", "
        << node_a_msg.get()->sequence_number() << std::endl;

  // 创建并发布NodeB消息
  auto node_b_msg = std::make_shared<IntegrationTestData>();
  node_b_msg->sensor_id(0);  // NodeB的sensor_id设为0
  node_b_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_b_msg->sequence_number(sequence_num_++);
  node_b_msg->data_size(64 * 1024);  // 64KB数据
  node_b_msg->data().resize(64 * 1024);

  // 填充64KB数据
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_b_msg->data()[i] = static_cast<unsigned char>('B');
  }

  node_b_writer_->Write(node_b_msg);

  AINFO << "NodeB published message #" << sequence_num_;

  return true;
}
