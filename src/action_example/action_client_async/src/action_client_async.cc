/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <chrono>

#include "example/action/LookUpTransform.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("action_client_async");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using ActionClient =
      rti::segar::action::ActionClient<example::action::LookUpTransform>;
  using GoalID = rti::segar::action::GoalID;
  using GoalStatusCode = rti::segar::action::GoalStatusCode;
  using LookUpTransform = example::action::LookUpTransform;

  ActionClient::GoalCallbacks callbacks;

  callbacks.on_feedback = [](ActionClient& /*client*/, const GoalID& goal_id,
                             const LookUpTransform::Feedback& feedback) {
    AINFO << "[Async] Feedback received: goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id)
          << ", current=" << feedback.current();
  };

  callbacks.on_result = [](ActionClient& /*client*/, const GoalID& goal_id,
                           const LookUpTransform::Result& result,
                           GoalStatusCode status) {
    AINFO << "[Async] Result received: goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id)
          << ", status=" << static_cast<int>(status)
          << ", transform=" << result.transform()
          << ", error=" << result.error();
  };

  // Register cancel callback.
  callbacks.on_cancel = [](ActionClient& /*client*/, const GoalID& goal_id,
                           rti::segar::action::CancelResponseCode code) {
    AINFO << "[Async] Cancel response: goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id)
          << ", code=" << static_cast<int>(code);
  };

  auto client =
      node->CreateActionClient<LookUpTransform>("lookup_transform", callbacks);
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto timer_callback = [&client, &index]() {
    ++index;
    LookUpTransform::Goal goal;
    goal.target_frame("map" + std::to_string(index));
    GoalID goal_id;
    if (!client->AsyncSendGoal(goal, &goal_id)) {
      AERROR << "[Async] SendGoal failed at iteration " << index;
      return;
    }
    AINFO << "[Async] Goal sent: iteration=" << index << ", goal_id="
          << rti::segar::action::internal::GoalIDToString(goal_id);

    // Cancel the current goal at a fixed interval.
    if (index > 0 && index % 5 == 0) {
      // Wait briefly so feedback has a chance to be received.
      rti::segar::SleepFor(std::chrono::milliseconds(30));
      AINFO << "[Async] Cancelling goal: goal_id="
            << rti::segar::action::internal::GoalIDToString(goal_id);
      client->AsyncCancelGoal(goal_id);
      return;
    }
  };

  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, timer_callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
