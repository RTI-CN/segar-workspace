/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

#include "integration_test/action/PerfAction.hpp"

#include "segar/segar.h"

namespace {
using rti::segar::action::ActionServer;
using rti::segar::action::GoalID;
using PerfAction = integration_test::action::PerfAction;

uint64_t NowUs() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
}  // namespace

int main(int argc, char** argv) {
  rti::segar::Init(*argv);

  auto node = rti::segar::CreateNode("perf_action_server");
  if (!node) {
    AERROR << "Failed to create node for action server.";
    return 1;
  }

  std::shared_ptr<ActionServer<PerfAction>> server;
  ActionServer<PerfAction>::Callbacks callbacks;
  callbacks.on_goal = [](ActionServer<PerfAction>&, const GoalID&,
                         const PerfAction::Goal& goal) {
    return goal.payload_size() == goal.payload().size();
  };
  callbacks.on_cancel = [](ActionServer<PerfAction>&, const GoalID&) {
    return true;
  };
  callbacks.on_execute =
      [&server](ActionServer<PerfAction>&, const GoalID& goal_id,
                const PerfAction::Goal& goal,
                const std::shared_ptr<std::atomic<bool>>& cancel_requested) {
        if (cancel_requested->load()) {
          return;
        }

        const uint64_t recv_ts_us = NowUs();
        auto result = std::make_shared<PerfAction::Result>();
        result->seq(goal.seq());
        result->payload_size(goal.payload_size());
        result->payload(std::move(goal.payload()));
        result->server_recv_ts_us(recv_ts_us);
        result->server_send_ts_us(NowUs());
        server->Succeed(goal_id, result);
      };

  rti::segar::action::ActionOptions option;
  option.max_server_active_goals = 4096;
  option.max_server_concurrent_goals = 16;
  server =
      node->CreateActionServer<PerfAction>("perf_action", callbacks, option);
  if (!server) {
    AERROR << "Failed to create action server.";
    return 1;
  }

  rti::segar::WaitForShutdown();
  return 0;
}
