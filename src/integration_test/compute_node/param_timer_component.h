/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_INTEGRATION_TEST_PARAM_TIMER_COMPONENT_H
#define SEGAR_EXAMPLES_INTEGRATION_TEST_PARAM_TIMER_COMPONENT_H

#include <cstdint>
#include <string>
#include <vector>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/component/timer_component.h"
#include "segar/action/action_client.h"
#include "integration_test/action/DemoAction.hpp"

class ParamTimerComponent : public rti::segar::TimerComponent {
 public:
  bool Init() override;
  bool Proc() override;

 private:
  bool ResolveRemoteParamsPath();
  bool EnsureDumpDir();

  std::string remote_params_path_;
  std::string dump_dir_;
  std::vector<std::string> node_names_;
  uint64_t tick_ = 0;
  using DemoAction=  integration_test::action::DemoAction;
  std::shared_ptr<rti::segar::action::ActionClient<DemoAction>>
      action_client_ = nullptr;
  rti::segar::action::GoalID action_goal_id_;
  bool action_goal_active_ = false;
  uint64_t action_tick_ = 0;
};

SEGAR_REGISTER_COMPONENT(ParamTimerComponent)

#endif  // SEGAR_EXAMPLES_INTEGRATION_TEST_PARAM_TIMER_COMPONENT_H
