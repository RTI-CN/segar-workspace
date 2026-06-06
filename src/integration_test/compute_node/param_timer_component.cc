/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "param_timer_component.h"

#include <utility>

#include "segar/common/file.h"
#include "segar/parameter/parameter.h"
#include "segar/parameter/segar_parameter_api.h"

namespace {
constexpr int kParamRefreshInterval = 10;
}  // namespace

bool ParamTimerComponent::Init() {
  node_names_ = {"node_c"};
  // node_names_ = {"node_a", "node_b", "node_c", "node_d",
  //                "node_e", "node_f", "node_g", "node_h"};
  remote_params_path_ = "params/node_c.yaml";
  dump_dir_ = "params/dump";

  ResolveRemoteParamsPath();
  EnsureDumpDir();

  action_client_ = node_->CreateActionClient<DemoAction>(
      "demo_action",
      rti::segar::action::ActionClient<DemoAction>::GoalCallbacks());

  AINFO << "ParamTimerComponent initialized";
  return true;
}

bool ParamTimerComponent::Proc() {
  ++tick_;

  const bool refresh = (tick_ % kParamRefreshInterval) == 0;
  for (const auto& node_name : node_names_) {
    if (refresh && !remote_params_path_.empty()) {
      Segar_Load_Remote_Params(node_name, remote_params_path_);
    }

    Segar_Set_Remote_Param(node_name, "p1_int", static_cast<int>(tick_));

    int p1_int = 0;
    Segar_Get_Remote_Param(node_name, "p1_int", &p1_int);
    AINFO << "ParamTimerComponent tick " << tick_ << " node " << node_name
          << " p1_int=" << p1_int;

    if (refresh) {
      std::vector<rti::segar::Parameter> fparameters;
      Segar_List_Remote_Params(node_name, &fparameters);

      for (const auto& parameter : fparameters) {
        AINFO << "ParamTimerComponent " << node_name << " "
              << parameter.DebugString();
      }

      if (!dump_dir_.empty()) {
        const std::string dump_file = dump_dir_ + "/" + node_name + ".yaml";
        Segar_Dump_Remote_Params(node_name, dump_file);
      }
    }
  }

  if (action_client_) {
    ++action_tick_;
    if (action_tick_ % 20 == 0) {
      DemoAction::Goal goal;
      goal.exec_times(8);
      rti::segar::action::ActionClient<DemoAction>::GoalCallbacks callbacks;
      callbacks.on_feedback =
          [](rti::segar::action::ActionClient<DemoAction>&,
             const rti::segar::action::GoalID&,
             const DemoAction::Feedback& feedback) {
        AINFO << "ACTION_ASYNC_FEEDBACK current=" << feedback.current();
      };
      callbacks.on_result =
          [](rti::segar::action::ActionClient<DemoAction>&,
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

    if (action_goal_active_ && action_tick_ % 25 == 0) {
      rti::segar::action::ActionClient<DemoAction>::GoalCallbacks cancel_cb;
      cancel_cb.on_cancel =
          [](rti::segar::action::ActionClient<DemoAction>&,
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

  return true;
}

bool ParamTimerComponent::ResolveRemoteParamsPath() {
  std::string resolved;
  if (rti::segar::common::GetFilePathWithEnv(remote_params_path_,
                                             "RTI_PARAM_PATH", &resolved)) {
    remote_params_path_ = resolved;
    return true;
  }
  AWARN << "ParamTimerComponent remote param file not found: "
        << remote_params_path_;
  return false;
}

bool ParamTimerComponent::EnsureDumpDir() {
  if (dump_dir_.empty()) {
    return false;
  }
  if (rti::segar::common::EnsureDirectory(dump_dir_)) {
    return true;
  }
  AWARN << "ParamTimerComponent failed to create dump dir: " << dump_dir_;
  return false;
}
