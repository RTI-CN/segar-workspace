/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>

#include "example/msg/String.hpp"

#include "segar/time/clock.h"
#include "segar/segar.h"

namespace {

using String = example::msg::String;

std::shared_ptr<String> MakeMessage(const std::string& data) {
  auto msg = std::make_shared<String>();
  msg->timestamp_ms(rti::segar::Clock::Now().ToMillisecond());
  msg->data(data);
  return msg;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (!rti::segar::Init(argv[0])) {
    return EXIT_FAILURE;
  }

  auto talker = rti::segar::CreateNode("node_lifecycle_talker");
  auto listener = rti::segar::CreateNode("node_lifecycle_listener");
  if (!talker || !listener) {
    return EXIT_FAILURE;
  }

  std::atomic<int> receive_count = {0};
  auto reader = listener->CreateReader<String>(
      "/topic/lifecycle_demo",
      [&receive_count](const std::shared_ptr<String>& msg) {
        ++receive_count;
        AINFO << "listener received message: " << msg->data();
      });
  auto writer = talker->CreateWriter<String>("/topic/lifecycle_demo");
  if (!reader || !writer) {
    return EXIT_FAILURE;
  }

  AINFO << "talker initial state: " << static_cast<int>(talker->GetState());
  AINFO << "listener initial state: " << static_cast<int>(listener->GetState());

  if (!writer->Write(MakeMessage("active_message_1"))) {
    AERROR << "failed to write active_message_1";
    return EXIT_FAILURE;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  listener->Deactivate();
  AINFO << "listener deactivated";
  if (!writer->Write(MakeMessage("listener_inactive_message"))) {
    AERROR << "failed to write listener_inactive_message";
    return EXIT_FAILURE;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  listener->Activate();
  talker->Deactivate();
  AINFO << "talker deactivated";
  if (writer->Write(MakeMessage("writer_inactive_message"))) {
    AERROR << "writer_inactive_message should not be sent while talker is inactive";
    return EXIT_FAILURE;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  talker->Activate();
  if (!writer->Write(MakeMessage("active_message_2"))) {
    AERROR << "failed to write active_message_2";
    return EXIT_FAILURE;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  AINFO << "final receive_count: " << receive_count.load();
  AINFO << "expected behavior: first and last messages are handled, middle two are blocked by lifecycle";
  return EXIT_SUCCESS;
}
