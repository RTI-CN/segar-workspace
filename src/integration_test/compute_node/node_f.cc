/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_f.h"

#include <chrono>

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeFComponent::Init() {
  // 创建发布者
  node_f_writer_ = node_->CreateWriter<IntegrationTestData>("NodeF");
  add_two_client_ = node_->CreateClient<AddTwoInts>("add_two_ints");
  action_client_ = node_->CreateActionClient<DemoAction>(
      "demo_action",
      rti::segar::action::ActionClient<DemoAction>::GoalCallbacks());

  // 初始化序列号
  sequence_num_ = 0;

  AINFO << "NodeFComponent initialized";
  return true;
}

bool NodeFComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_c1_msg,
    const std::shared_ptr<IntegrationTestData>& node_c2_msg,
    const std::shared_ptr<IntegrationTestData>& node_c3_msg,
    const std::shared_ptr<IntegrationTestData>& node_c4_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_c1_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeF input=NodeC1 seq="
        << node_c1_msg->sequence_number() << " latency_ns=" << latency_ns;

  // if (action_client_) {
  ++action_seq_;
  if (action_seq_ % 25 == 0) {
    DemoAction::Goal goal;
    goal.exec_times(5);
    rti::segar::action::GoalID goal_id;
    if (action_client_->SyncSendGoal(goal, &goal_id)) {
      DemoAction::Result result;
      rti::segar::action::GoalStatusCode status;
      if (action_client_->WaitForResult(goal_id, &result, &status)) {
        AINFO << "ACTION_SYNC_OK total=" << result.total()
              << " status=" << static_cast<int>(status);
      } else {
        AINFO << "ACTION_SYNC_TIMEOUT";
      }
    } else {
      AINFO << "ACTION_SYNC_SEND_FAIL";
    }
  }

  if (action_seq_ % 40 == 0) {
    DemoAction::Goal goal;
    goal.exec_times(20);
    rti::segar::action::ActionClient<DemoAction>::GoalCallbacks callbacks;
    callbacks.on_feedback = [](rti::segar::action::ActionClient<DemoAction>&,
                               const rti::segar::action::GoalID&,
                               const DemoAction::Feedback& feedback) {
      AINFO << "ACTION_ASYNC_FEEDBACK current=" << feedback.current();
    };
    callbacks.on_result = [](rti::segar::action::ActionClient<DemoAction>&,
                             const rti::segar::action::GoalID&,
                             const DemoAction::Result& result,
                             rti::segar::action::GoalStatusCode status) {
      AINFO << "ACTION_ASYNC_RESULT total=" << result.total()
            << " status=" << static_cast<int>(status);
    };
    if (action_client_->AsyncSendGoal(goal, &action_goal_id_, callbacks)) {
      action_goal_active_ = true;
      AINFO << "ACTION_ASYNC_SENT";
    } else {
      AINFO << "ACTION_ASYNC_SEND_FAIL";
    }
  }

  // 处理接收的消息（此处仅为示例）
  AINFO << "NodeF received messages :::" << node_c1_msg.get()->sequence_number()
        << ", " << node_c2_msg.get()->sequence_number() << ", "
        << node_c3_msg.get()->sequence_number() << ", "
        << node_c4_msg.get()->sequence_number() << std::endl;
  cpu_burner_ms(25);

  // 创建并发布NodeF消息
  auto node_f_msg = std::make_shared<IntegrationTestData>();
  node_f_msg->sensor_id(0);  // NodeF的sensor_id设为0
  node_f_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_f_msg->sequence_number(sequence_num_++);
  node_f_msg->data_size(64 * 1024);  // 64KB数据
  node_f_msg->data().resize(64 * 1024);

  // 填充64KB数据
  for (size_t i = 0; i < 64 * 1024; ++i) {
    node_f_msg->data()[i] = static_cast<unsigned char>('B');
  }
  node_f_writer_->Write(node_f_msg);

  AINFO << "NodeF published message #" << sequence_num_;

  return true;
}

SEGAR_REGISTER_COMPONENT(NodeFComponent)
