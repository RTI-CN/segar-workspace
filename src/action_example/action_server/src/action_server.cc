/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <atomic>
#include <memory>

#include "example/action/LookUpTransform.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("action_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using ActionServer =
      rti::segar::action::ActionServer<example::action::LookUpTransform>;
  using GoalID = rti::segar::action::GoalID;
  using LookUpTransform = example::action::LookUpTransform;

  std::shared_ptr<ActionServer> server;

  ActionServer::Callbacks callbacks;

  callbacks.on_goal = [](ActionServer& /*server*/, const GoalID& /*goal_id*/,
                         const LookUpTransform::Goal& goal) -> bool {
    AINFO << "Accepted goal: target_frame=" << goal.target_frame();
    return true;
  };

  callbacks.on_cancel = [](ActionServer& /*server*/,
                           const GoalID& goal_id) -> bool {
    AINFO << "Cancel request received for goal_id: "
          << rti::segar::action::internal::GoalIDToString(goal_id);
    return true;
  };

  callbacks.on_execute =
      [&server](ActionServer& /*server_ref*/, const GoalID& goal_id,
                const LookUpTransform::Goal& goal,
                const std::shared_ptr<std::atomic<bool>>& cancel_requested) {
        auto result =
            std::make_shared<example::action::LookUpTransform::Result>();

        const auto& target_frame = goal.target_frame();
        AINFO << "Executing target_frame: " << target_frame;

        constexpr int32_t steps = 5;
        for (int32_t i = 1; i <= steps; ++i) {
          AINFO << "Goal executing at step " << i << "/" << steps;

          auto feedback =
              std::make_shared<example::action::LookUpTransform::Feedback>();
          feedback->current(i);
          if (server) {
            server->PublishFeedback(goal_id, feedback);
          }

          if (cancel_requested->load()) {
            AINFO << "Goal cancelled at step " << i << "/" << steps;
            result->error(-1);
            result->transform("cancelled");
            if (server) {
              server->CancelGoal(goal_id, result);
            }
            return;
          }

          rti::segar::SleepFor(std::chrono::milliseconds(20));
        }

        result->transform("transform_from_" + target_frame);
        result->error(0);
        AINFO << "Goal completed successfully: target_frame=" << target_frame;
        if (server) {
          server->Succeed(goal_id, result);
        }
      };

  server =
      node->CreateActionServer<LookUpTransform>("lookup_transform", callbacks);
  RETURN_VAL_IF(!server, EXIT_FAILURE);

  AINFO << "Action server started successfully";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
