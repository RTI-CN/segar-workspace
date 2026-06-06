/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <cstdint>
#include <memory>

#include "example/msg/Image.hpp"

#include "segar/segar.h"
#include "segar/time/time.h"

namespace {

using example::msg::Image;
using rti::segar::Time;

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("zero_copy_listener");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  auto reader = node->CreateReader<Image>(
      "/topic/zero_copy",
      [](const std::shared_ptr<Image>& msg) {
        if (msg == nullptr) {
          AWARN << "received null zero-copy image";
          return;
        }
        const uint64_t now_ms = Time::Now().ToMillisecond();
        const uint64_t latency_ms =
            now_ms >= msg->timestamp_ms() ? (now_ms - msg->timestamp_ms()) : 0;
        AINFO << "Received zero-copy image: width=" << msg->width()
              << ", latency_ms=" << latency_ms;
      });
  RETURN_VAL_IF(!reader, EXIT_FAILURE);

  AINFO << "Waiting for zero-copy images...";
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
