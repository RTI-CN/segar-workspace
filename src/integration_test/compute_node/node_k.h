/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_K_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_K_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"

using rti::segar::Component;
using rti::segar::Writer;
using integration_test::msg::IntegrationTestData;

class NodeKComponent : public Component<IntegrationTestData> {
 public:
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& node_j_msg) override;

 private:
  std::shared_ptr<Writer<IntegrationTestData>> node_k_writer_ = nullptr;
  uint64_t sequence_num_ = 0;
};
SEGAR_REGISTER_COMPONENT(NodeKComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_K_H
