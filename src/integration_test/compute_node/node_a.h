/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_A_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_A_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"
#include "segar/srv/AddTwoInts.hpp" 
#include "segar/segar.h"
#include "segar/service/client.h"
#include "segar/time/rate.h"
#include "segar/time/time.h"

using rti::segar::Component;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

class NodeAComponent
    : public Component<IntegrationTestData, IntegrationTestData,
                       IntegrationTestData, IntegrationTestData> {
 public:
  using AddTwoInts =  segar::srv::AddTwoInts;
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& sensor1_msg,
            const std::shared_ptr<IntegrationTestData>& sensor4_msg,
            const std::shared_ptr<IntegrationTestData>& sensor6_msg,
            const std::shared_ptr<IntegrationTestData>& sensor7_msg) override;

 private:
  std::shared_ptr<Writer<IntegrationTestData>> node_a_writer_ = nullptr;
  std::shared_ptr<rti::segar::service::Client<AddTwoInts>> add_two_client_ =
      nullptr;
  uint64_t add_two_seq_ = 0;
  uint64_t sequence_num_;
};
SEGAR_REGISTER_COMPONENT(NodeAComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_A_H
