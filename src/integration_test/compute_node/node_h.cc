/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_h.h"

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeHComponent::Init() {
  add_two_service_ = node_->CreateService<AddTwoInts>(
      "add_two_ints", [](const std::shared_ptr<AddTwoInts::Request>& request,
                         std::shared_ptr<AddTwoInts::Response>& response) {
        response->sum(request->a() + request->b());
        AINFO << "NodeH add_two_ints request a=" << request->a()
              << " b=" << request->b() << " sum=" << response->sum();
      });
  action_client_ = node_->CreateActionClient<DemoAction>(
      "demo_action",
      rti::segar::action::ActionClient<DemoAction>::GoalCallbacks());
  AINFO << "NodeHComponent initialized";
  return true;
}

bool NodeHComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_a_msg,
    const std::shared_ptr<IntegrationTestData>& node_b_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_a_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeH input=NodeA seq="
        << node_a_msg->sequence_number() << " latency_ns=" << latency_ns;

  // 处理接收的消息（此处仅为示例）
  AINFO << "NodeH received messages :::" << node_a_msg.get()->sequence_number()
        << ", " << node_b_msg.get()->sequence_number() << std::endl;

  if (action_client_) {
    ++action_seq_;
    if (action_seq_ % 30 == 0) {
      DemoAction::Goal goal;
      goal.exec_times(10);
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

    if (action_goal_active_ && action_seq_ % 35 == 0) {
      rti::segar::action::ActionClient<DemoAction>::GoalCallbacks cancel_cb;
      cancel_cb.on_cancel = [](rti::segar::action::ActionClient<DemoAction>&,
                               const rti::segar::action::GoalID&,
                               rti::segar::action::CancelResponseCode code) {
        AINFO << "ACTION_CANCEL code=" << static_cast<int>(code);
      };
      if (action_client_->AsyncCancelGoal(action_goal_id_, cancel_cb)) {
        AINFO << "ACTION_CANCEL_SENT";
      } else {
        AINFO << "ACTION_CANCEL_FAIL";
      }
      action_goal_active_ = false;
    }
  }
  cpu_burner_ms(25);

  return true;
}

SEGAR_REGISTER_COMPONENT(NodeHComponent)
