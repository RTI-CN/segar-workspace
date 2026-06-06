/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_L_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_L_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"

using rti::segar::Component;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

class NodeLComponent
    : public Component<IntegrationTestData, IntegrationTestData> {
 public:
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& node_f_msg,
            const std::shared_ptr<IntegrationTestData>& node_k_msg) override;

 private:
  std::shared_ptr<Writer<IntegrationTestData>> node_l_writer_ = nullptr;
  uint64_t sequence_num_ = 0;
};
SEGAR_REGISTER_COMPONENT(NodeLComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_L_H
