/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_G_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_G_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"
#include "segar/action/action_server.h"
#include "integration_test/action/DemoAction.hpp"

using rti::segar::Component;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

class NodeGComponent
    : public Component<IntegrationTestData, IntegrationTestData,
                       IntegrationTestData, IntegrationTestData> {
 public:
  using DemoAction=  integration_test::action::DemoAction;
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& node_c1_msg,
            const std::shared_ptr<IntegrationTestData>& node_c2_msg,
            const std::shared_ptr<IntegrationTestData>& node_c3_msg,
            const std::shared_ptr<IntegrationTestData>& node_c4_msg) override;

 private:
  std::shared_ptr<Writer<IntegrationTestData>> node_g_writer_ = nullptr;
  std::shared_ptr<rti::segar::action::ActionServer<DemoAction>>
      action_server_ = nullptr;
  uint64_t sequence_num_;
};
SEGAR_REGISTER_COMPONENT(NodeGComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_G_H
