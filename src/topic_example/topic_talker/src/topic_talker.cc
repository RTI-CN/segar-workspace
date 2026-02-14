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
  auto node = rti::segar::CreateNode("topic_talker");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto writer = node->CreateWriter<example::msg::String>("/topic/chatter");
  RETURN_VAL_IF(!writer, EXIT_FAILURE);
  uint32_t seq = 0;
  auto callback = [&writer, &seq]() {
    auto msg = std::make_shared<example::msg::String>();
    msg->data(std::to_string(seq++));
    AINFO_IF(!writer->Write(msg)) << "Failed to write msg:" << msg->data();
    AINFO << "Sent message: " << msg->data();
  };
  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
