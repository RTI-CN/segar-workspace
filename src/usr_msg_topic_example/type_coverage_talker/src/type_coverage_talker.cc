/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <cstdint>
#include <memory>

#include "test_msgs/msg/TypeCoverage.hpp"

#include "segar/segar.h"
#include "segar/time/clock.h"

namespace {

using TypeCoverage = test_msgs::msg::TypeCoverage;

constexpr char kTopic[] = "/topic/usr_msg/type_coverage";

std::shared_ptr<TypeCoverage> MakeTypeCoverage(uint32_t seq) {
  auto msg = std::make_shared<TypeCoverage>();
  msg->ready((seq % 2) == 0);
  msg->int8_value(static_cast<int8_t>(seq));
  msg->uint8_value(static_cast<uint8_t>(seq));
  msg->int16_value(static_cast<int16_t>(seq));
  msg->uint16_value(static_cast<uint16_t>(seq));
  msg->int32_value(static_cast<int32_t>(seq));
  msg->uint32_value(seq);
  msg->int64_value(static_cast<int64_t>(seq));
  msg->uint64_value(rti::segar::Time::Now().ToNanosecond());
  msg->temperature(25.0f + static_cast<float>(seq) * 0.1f);
  msg->voltage(3.3 + static_cast<double>(seq) * 0.05);
  msg->note("sample-" + std::to_string(seq));

  msg->raw_bytes().assign({0xA0, 0xB0, 0xC0});
  msg->py_bytes().assign({0xA0, 0xB0, 0xC0});
  msg->py_bytearray().assign({static_cast<int8_t>(-5), static_cast<int8_t>(-4),
                              static_cast<int8_t>(-3)});
  msg->py_memoryview().assign({0xD0, 0xD1, 0xD2});
  msg->fixed_bytes()[0] = 1;
  msg->fixed_bytes()[1] = 2;
  msg->fixed_bytes()[2] = 3;
  for (size_t i = 0; i < msg->samples().size(); ++i) {
    msg->samples()[i] = static_cast<int32_t>(seq * 10 + i);
  }
  msg->detail().detail_id(static_cast<int32_t>(seq));
  msg->detail().description("detail-" + std::to_string(seq));
  return msg;
}

}  // namespace

int main(int argc, char* argv[]) {
  (void)argc;
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);
  auto node = rti::segar::CreateNode("usr_msg_type_coverage_talker");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  auto writer = node->CreateWriter<TypeCoverage>(kTopic);
  RETURN_VAL_IF(!writer, EXIT_FAILURE);

  uint32_t seq = 1;
  auto callback = [&writer, &seq]() {
    auto msg = MakeTypeCoverage(seq++);
    AINFO_IF(!writer->Write(msg)) << "Failed to write TypeCoverage";
    AINFO << "Sent TypeCoverage int32=" << msg->int32_value()
          << ", note=" << msg->note();
  };

  auto timer = std::make_shared<rti::segar::Timer>(1000, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
