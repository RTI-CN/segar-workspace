/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_C_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_C_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"
#include "segar/srv/AddTwoInts.hpp" 
#include "segar/service/client.h"

using rti::segar::Component;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

class NodeCComponent
    : public Component<IntegrationTestData, IntegrationTestData,
                       IntegrationTestData, IntegrationTestData,
                       IntegrationTestData> {
 public:
  using AddTwoInts =  segar::srv::AddTwoInts;
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& sensor8_msg,
            const std::shared_ptr<IntegrationTestData>& sensor9_msg,
            const std::shared_ptr<IntegrationTestData>& sensor10_msg,
            const std::shared_ptr<IntegrationTestData>& node_a_msg,
            const std::shared_ptr<IntegrationTestData>& node_b_msg) override;

 private:
  std::shared_ptr<Writer<IntegrationTestData>> node_c1_writer_ = nullptr;
  std::shared_ptr<Writer<IntegrationTestData>> node_c2_writer_ = nullptr;
  std::shared_ptr<Writer<IntegrationTestData>> node_c3_writer_ = nullptr;
  std::shared_ptr<Writer<IntegrationTestData>> node_c4_writer_ = nullptr;
  std::shared_ptr<rti::segar::service::Client<AddTwoInts>> add_two_client_ =
      nullptr;
  uint64_t add_two_seq_ = 0;
  uint64_t sequence_num_;
};
SEGAR_REGISTER_COMPONENT(NodeCComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_C_H
