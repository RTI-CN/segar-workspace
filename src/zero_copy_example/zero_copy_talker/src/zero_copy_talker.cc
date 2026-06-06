/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <cstddef>
#include <cstdint>

#include "example/msg/Image.hpp"

#include "segar/segar.h"
#include "segar/time/rate.h"
#include "segar/time/time.h"

namespace {

using example::msg::Image;
using rti::segar::Rate;
using rti::segar::Time;

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("zero_copy_talker");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  auto writer = node->CreateWriter<Image>("/topic/zero_copy");
  RETURN_VAL_IF(!writer, EXIT_FAILURE);

  Rate rate(2.0);
  uint64_t width = 0;
  while (rti::segar::OK()) {
    auto msg = writer->LoanSample();
    if (!msg) {
      AWARN << "LoanSample failed";
      rate.Sleep();
      continue;
    }

    msg->width(static_cast<int32_t>(width));
    msg->timestamp_ms(Time::Now().ToMillisecond());

    if (!writer->Write(msg)) {
      AWARN << "Failed to write zero-copy image, width=" << width;
    } else {
      AINFO << "Sent zero-copy image: width=" << width;
    }
    ++width;
    rate.Sleep();
  }

  return EXIT_SUCCESS;
}
