/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <chrono>
#include <memory>
#include <string>

#include "example/msg/String.hpp"

#include "segar/segar.h"

using rti::segar::WaitEvent;
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

  auto node = rti::segar::CreateNode("wait_event_talker");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  auto ready_event = std::make_shared<WaitEvent>();
  auto request_writer = node->CreateWriter<example::msg::String>(
      MakeTransientLocalAttr("/topic/wait_event/request"));
  RETURN_VAL_IF(!request_writer, EXIT_FAILURE);

  auto ack_reader = node->CreateReader<example::msg::String>(
      MakeTransientLocalAttr("/topic/wait_event/ack"),
      [ready_event](const std::shared_ptr<example::msg::String>& msg) {
        if (!msg || msg->data() != "ready") {
          return;
        }
        AINFO << "Received ready ack";
        ready_event->Notify();
      });
  RETURN_VAL_IF(!ack_reader, EXIT_FAILURE);

  auto worker = rti::segar::Async([request_writer, ack_reader, ready_event]() {
    AINFO_IF(!request_writer->Write(MakeMessage("prepare")))
        << "Failed to send prepare";
    AINFO << "Sent prepare and waiting for ready ack";

    if (!ready_event->WaitFor(std::chrono::seconds(5))) {
      AWARN << "Timed out waiting for ready ack";
      return;
    }

    AINFO_IF(!request_writer->Write(MakeMessage("go"))) << "Failed to send go";
    AINFO << "Sent go after ready ack";
  });

  RETURN_VAL_IF(!worker.valid(), EXIT_FAILURE);
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
