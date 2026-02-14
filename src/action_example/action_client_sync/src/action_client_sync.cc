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
namespace {
using ActionClient =
    rti::segar::action::ActionClient<example::action::LookUpTransform>;
using ActionClientSPtr = std::shared_ptr<ActionClient>;
using GoalID = rti::segar::action::GoalID;
using GoalStatusCode = rti::segar::action::GoalStatusCode;
using LookUpTransform = example::action::LookUpTransform;

void SyncCancelAfterSendGoal(ActionClientSPtr& client, const int32_t index) {
  LookUpTransform::Goal goal;
  goal.target_frame("map" + std::to_string(index));
  GoalID goal_id;
  if (!client->SyncSendGoal(goal, &goal_id)) {
    AERROR << "[Sync] SyncSendGoal failed at iteration " << index;
    return;
  }

  AINFO << "[Sync] Goal sent: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);
  // Wait until goal execution has actually started.
  rti::segar::SleepFor(std::chrono::milliseconds(50));
  if (!client->SyncCancelGoal(goal_id)) {
    AERROR << "[Sync] CancelGoal failed, goal_id="
           << rti::segar::action::internal::GoalIDToString(goal_id);
    return;
  }
  AINFO << "[Sync] Cancel request sent: goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);

  if (!client->WaitForResult(goal_id)) {
    AERROR << "[Sync]cancel failed, goal_id="
           << rti::segar::action::internal::GoalIDToString(goal_id);
    return;
  }
  AINFO << "[Sync] cancel success: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);
}
}  // namespace

void SyncSendGoal(ActionClientSPtr& client, const int32_t index) {
  LookUpTransform::Goal goal;
  goal.target_frame("map" + std::to_string(index));
  GoalID goal_id;
  if (!client->SyncSendGoal(goal, &goal_id)) {
    AERROR << "[Sync] SyncSendGoal failed at iteration " << index;
    return;
  }

  AINFO << "[Sync] Goal sent: iteration=" << index << ", goal_id="
        << rti::segar::action::internal::GoalIDToString(goal_id);
  LookUpTransform::Result result;
  GoalStatusCode status = GoalStatusCode::STATUS_UNKNOWN;
  if (!client->WaitForResult(goal_id, &result, &status)) {
    AERROR << "WaitForResult failed at index=" << index;
    return;
  }
  AINFO << "[Sync] Result received: iteration=" << index
        << ", goal_id=" << rti::segar::action::internal::GoalIDToString(goal_id)
        << ", status=" << static_cast<int>(status)
        << ", transform=" << result.transform() << ", error=" << result.error();
}

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("action_client_sync");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto client = node->CreateActionClient<LookUpTransform>("lookup_transform");
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto callback = [&client, &index]() {
    index++;
    // Cancel the currently sent goal at a fixed interval.
    if (index > 0 && index % 2 == 0) {
      SyncCancelAfterSendGoal(client, index);
      return;
    }
    SyncSendGoal(client, index);
  };

  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
