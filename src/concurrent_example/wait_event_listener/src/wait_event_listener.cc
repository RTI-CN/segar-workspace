/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <memory>
#include <string>

#include "example/msg/String.hpp"

#include "segar/segar.h"

using rti::segar::proto::QosDurabilityPolicy;
using rti::segar::proto::RoleAttributes;

namespace {

std::shared_ptr<example::msg::String> MakeMessage(const std::string& data) {
  auto msg = std::make_shared<example::msg::String>();
  msg->data(data);
  return msg;
}

RoleAttributes MakeTransientLocalAttr(const std::string& topic_name) {
  RoleAttributes attr;
  attr.set_topic_name(topic_name);
  attr.mutable_qos_profile()->set_durability(
      QosDurabilityPolicy::DURABILITY_TRANSIENT_LOCAL);
  return attr;
}

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("wait_event_listener");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  auto ack_writer = node->CreateWriter<example::msg::String>(
      MakeTransientLocalAttr("/topic/wait_event/ack"));
  RETURN_VAL_IF(!ack_writer, EXIT_FAILURE);

  auto request_reader = node->CreateReader<example::msg::String>(
      MakeTransientLocalAttr("/topic/wait_event/request"),
      [ack_writer](const std::shared_ptr<example::msg::String>& msg) {
        if (!msg) {
          return;
        }
        if (msg->data() == "prepare") {
          AINFO << "Received prepare";
          AINFO_IF(!ack_writer->Write(MakeMessage("ready")))
              << "Failed to send ready ack";
          AINFO << "Sent ready ack";
          return;
        }
        if (msg->data() == "go") {
          AINFO << "Received go and can continue work";
        }
      });
  RETURN_VAL_IF(!request_reader, EXIT_FAILURE);

  AINFO << "Waiting for wait_event requests...";
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
