/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_F_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_F_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"
#include "segar/srv/AddTwoInts.hpp" 
#include "segar/service/client.h"
#include "segar/action/action_client.h"
#include "integration_test/action/DemoAction.hpp"

using rti::segar::Component;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

class NodeFComponent
    : public Component<IntegrationTestData, IntegrationTestData,
                       IntegrationTestData, IntegrationTestData> {
 public:
  using AddTwoInts =  segar::srv::AddTwoInts;
  using DemoAction=  integration_test::action::DemoAction;
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& node_c1_msg,
            const std::shared_ptr<IntegrationTestData>& node_c2_msg,
            const std::shared_ptr<IntegrationTestData>& node_c3_msg,
            const std::shared_ptr<IntegrationTestData>& node_c4_msg) override;

 private:
  std::shared_ptr<Writer<IntegrationTestData>> node_f_writer_ = nullptr;
  std::shared_ptr<rti::segar::service::Client<AddTwoInts>> add_two_client_ =
      nullptr;
  uint64_t add_two_seq_ = 0;
  std::shared_ptr<rti::segar::action::ActionClient<DemoAction>>
      action_client_ = nullptr;
  rti::segar::action::GoalID action_goal_id_;
  bool action_goal_active_ = false;
  uint64_t action_seq_ = 0;
  uint64_t sequence_num_;
};
SEGAR_REGISTER_COMPONENT(NodeFComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_F_H
