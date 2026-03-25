/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <cstdint>
#include <memory>

#include "example/msg/Image.hpp"

#include "segar/message/loaned_message.h"
#include "segar/segar.h"
#include "segar/time/time.h"

namespace {

using LoanedZeroCopySample =
    rti::segar::message::LoanedMessage<example::msg::Image>;
using rti::segar::Time;

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("zero_copy_listener");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  auto reader = node->CreateReader<LoanedZeroCopySample>(
      "/topic/zero_copy",
      [](const std::shared_ptr<LoanedZeroCopySample>& msg) {
        if (msg == nullptr || msg->GetMessage() == nullptr) {
          AWARN << "received null loaned message";
          return;
        }
        const auto* data = msg->GetMessage();
        const uint64_t now_ms = Time::Now().ToMillisecond();
        const uint64_t latency_ms =
            now_ms >= data->timestamp_ms() ? (now_ms - data->timestamp_ms()) : 0;
        AINFO << "Received loaned zero-copy image: width=" << data->width()
              << ", latency_ms=" << latency_ms;
      });
  RETURN_VAL_IF(!reader, EXIT_FAILURE);

  AINFO << "Waiting for loaned zero-copy images...";
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
