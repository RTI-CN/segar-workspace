/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>
#include <string>

#include "example/msg/String.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("topic_listener");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto reader = node->CreateReader<example::msg::String>(
      "/topic/chatter",
      [](const auto& msg) { AINFO << "Received message: " << msg->data(); });
  RETURN_VAL_IF(!reader, EXIT_FAILURE);
  AINFO << "Waiting for messages...";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
