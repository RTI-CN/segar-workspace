/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_c.h"

#include <chrono>

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeCComponent::Init() {
  // 创建发布者
  node_c1_writer_ = node_->CreateWriter<IntegrationTestData>("NodeC1");
  node_c2_writer_ = node_->CreateWriter<IntegrationTestData>("NodeC2");
  node_c3_writer_ = node_->CreateWriter<IntegrationTestData>("NodeC3");
  node_c4_writer_ = node_->CreateWriter<IntegrationTestData>("NodeC4");
  add_two_client_ = node_->CreateClient<AddTwoInts>("add_two_ints");

  // 初始化序列号
  sequence_num_ = 0;

  AINFO << "NodeCComponent initialized";
  return true;
}

bool NodeCComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& sensor8_msg,
    const std::shared_ptr<IntegrationTestData>& sensor9_msg,
    const std::shared_ptr<IntegrationTestData>& sensor10_msg,
    const std::shared_ptr<IntegrationTestData>& node_a_msg,
    const std::shared_ptr<IntegrationTestData>& node_b_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - sensor8_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeC input=sensor_topic_8 seq="
        << sensor8_msg->sequence_number() << " latency_ns=" << latency_ns;

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
        AINFO << "NodeC sync add_two_ints response=" << res->sum();
      } else {
        AINFO << "NodeC sync add_two_ints timeout or not ready";
      }

      auto async_request = std::make_shared<AddTwoInts::Request>();
      async_request->a(static_cast<int64_t>(add_two_seq_));
      async_request->b(static_cast<int64_t>(add_two_seq_ + 100));
      rti::segar::service::Client<AddTwoInts>::ResponseCallback callback =
          [](const std::shared_ptr<AddTwoInts::Response>& response) {
            if (response) {
              AINFO << "NodeC async add_two_ints response=" << response->sum();
            } else {
              AINFO << "NodeC async add_two_ints timeout or not ready";
            }
          };
      rti::segar::service::Client<AddTwoInts>::RequestHandle handle;
      add_two_client_->AsyncSendRequest(async_request, callback, {}, &handle);
    }
  }

  // 处理接收的消息（此处仅为示例）
  AINFO << "NodeC received messages :::" << sensor8_msg.get()->sequence_number()
        << ", " << sensor9_msg.get()->sequence_number() << ", "
        << sensor10_msg.get()->sequence_number() << ", "
        << node_a_msg.get()->sequence_number() << ", "
        << node_b_msg.get()->sequence_number() << std::endl;

  cpu_burner_ms(25);

  // 创建并发布NodeC消息 (NodeC1, NodeC2, NodeC3, NodeC4)
  auto node_c1_msg = std::make_shared<IntegrationTestData>();
  auto node_c2_msg = std::make_shared<IntegrationTestData>();
  auto node_c3_msg = std::make_shared<IntegrationTestData>();
  auto node_c4_msg = std::make_shared<IntegrationTestData>();

  // NodeC1消息
  node_c1_msg->sensor_id(0);

  node_c1_msg->sequence_number(sequence_num_);
  node_c1_msg->data_size(64 * 1024);  // 64KB数据
  node_c1_msg->data().resize(64 * 1024);
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_c1_msg->data()[i] = static_cast<unsigned char>('B');
  }

  // NodeC2消息
  node_c2_msg->sensor_id(0);
  node_c2_msg->sequence_number(sequence_num_);
  node_c2_msg->data_size(64 * 1024);  // 64KB数据
  node_c2_msg->data().resize(64 * 1024);
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_c2_msg->data()[i] = static_cast<unsigned char>('B');
  }

  // NodeC3消息
  node_c3_msg->sensor_id(0);
  node_c3_msg->sequence_number(sequence_num_);
  node_c3_msg->data_size(64 * 1024);  // 64KB数据
  node_c3_msg->data().resize(64 * 1024);
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_c3_msg->data()[i] = static_cast<unsigned char>('B');
  }

  // NodeC4消息
  node_c4_msg->sensor_id(0);
  node_c4_msg->sequence_number(sequence_num_);
  node_c4_msg->data_size(64 * 1024);  // 64KB数据
  node_c4_msg->data().resize(64 * 1024);
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_c4_msg->data()[i] = static_cast<unsigned char>('B');
  }
  uint64_t timestamp = rti::segar::Time::Now().ToNanosecond();
  node_c1_msg->timestamp(timestamp);
  node_c2_msg->timestamp(timestamp);
  node_c3_msg->timestamp(timestamp);
  node_c4_msg->timestamp(timestamp);
  node_c1_writer_->Write(node_c1_msg);
  node_c2_writer_->Write(node_c2_msg);
  node_c3_writer_->Write(node_c3_msg);
  node_c4_writer_->Write(node_c4_msg);

  sequence_num_++;
  AINFO << "NodeC published messages #" << sequence_num_;

  return true;
}
