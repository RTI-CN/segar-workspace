/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_g.h"

#include <atomic>
#include <vector>

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"
bool NodeGComponent::Init() {
  // 创建发布者
  node_g_writer_ = node_->CreateWriter<IntegrationTestData>("NodeG");

  // 初始化序列号
  sequence_num_ = 0;

  using ActionServer = rti::segar::action::ActionServer<DemoAction>;
  ActionServer::Callbacks callbacks;
  callbacks.on_goal = [](ActionServer& /*server*/,
                         const rti::segar::action::GoalID&,
                         const DemoAction::Goal& goal) {
    AINFO << "ACTION_SERVER_GOAL exec_times=" << goal.exec_times();
    return goal.exec_times() > 0;
  };
  callbacks.on_cancel = [](ActionServer& /*server*/,
                           const rti::segar::action::GoalID&) {
    AINFO << "ACTION_SERVER_CANCEL request";
    return true;
  };
  callbacks.on_execute =
      [this](ActionServer& /*server*/,
             const rti::segar::action::GoalID& goal_id,
             const DemoAction::Goal& goal,
             const std::shared_ptr<std::atomic<bool>>& cancel_requested) {
        if (!action_server_) {
          return;
        }
        auto feedback = std::make_shared<DemoAction::Feedback>();
        auto result = std::make_shared<DemoAction::Result>();
        std::vector<int32_t> sequence;
        int32_t exec_times = goal.exec_times();
        for (int32_t i = 1; i <= exec_times; ++i) {
          if (cancel_requested->load()) {
            result->total(i - 1);
            integration_test::msg::ResultDetail partial_detail;
            partial_detail.full_sequence(sequence);
            result->detail(partial_detail);
            action_server_->CancelGoal(goal_id, result);
            AINFO << "ACTION_SERVER_CANCELED total=" << result->total();
            return;
          }
          sequence.push_back(i);
          feedback->current(i);
          action_server_->PublishFeedback(goal_id, feedback);
          // rti::segar::SleepFor(std::chrono::milliseconds(200));
        }

        result->total(exec_times);
        integration_test::msg::ResultDetail final_detail;
        final_detail.full_sequence(sequence);
        result->detail(final_detail);
        action_server_->Succeed(goal_id, result);
        AINFO << "ACTION_SERVER_SUCCEED total=" << result->total();
      };

  action_server_ = node_->CreateActionServer<DemoAction>(
      "demo_action", callbacks, rti::segar::action::ActionOptions());

  AINFO << "NodeGComponent initialized";
  return true;
}

bool NodeGComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_b_msg,
    const std::shared_ptr<IntegrationTestData>& node_d_msg,
    const std::shared_ptr<IntegrationTestData>& node_e_msg,
    const std::shared_ptr<IntegrationTestData>& node_f_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_b_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeG input=NodeB seq="
        << node_b_msg->sequence_number() << " latency_ns=" << latency_ns;

  // 处理接收的消息（此处仅为示例）
  AINFO << "NodeG received messages :::" << node_b_msg.get()->sequence_number()
        << ", " << node_d_msg.get()->sequence_number() << ", "
        << node_e_msg.get()->sequence_number() << ", "
        << node_f_msg.get()->sequence_number() << std::endl;
  // cpu_burner_ms(1);
  return true;
}

SEGAR_REGISTER_COMPONENT(NodeGComponent)
