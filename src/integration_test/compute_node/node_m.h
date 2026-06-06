/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#ifndef SEGAR_EXAMPLES_COMPONENT_TEST_NODE_M_H
#define SEGAR_EXAMPLES_COMPONENT_TEST_NODE_M_H

#include <memory>

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "integration_test/msg/IntegrationTestData.hpp"

using rti::segar::Component;
using integration_test::msg::IntegrationTestData;

class NodeMComponent : public Component<IntegrationTestData> {
 public:
  bool Init() override;
  bool Proc(const std::shared_ptr<IntegrationTestData>& node_l_msg) override;
};
SEGAR_REGISTER_COMPONENT(NodeMComponent)
#endif  // SEGAR_EXAMPLES_COMPONENT_TEST_NODE_M_H
