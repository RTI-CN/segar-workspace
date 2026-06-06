/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include <sstream>

#include "test_msgs/msg/TypeCoverage.hpp"

#include "segar/segar.h"

namespace {

using TypeCoverage = test_msgs::msg::TypeCoverage;

constexpr char kTopic[] = "/topic/usr_msg/type_coverage";

template <typename Container>
std::string ValuesToString(const Container& values) {
  std::ostringstream stream;
  stream << "[";
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << static_cast<int>(values[i]);
  }
  stream << "]";
  return stream.str();
}

}  // namespace

int main(int argc, char* argv[]) {
  (void)argc;
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("usr_msg_type_coverage_listener");
  RETURN_VAL_IF(!node, EXIT_FAILURE);
  auto reader = node->CreateReader<TypeCoverage>(
      kTopic, [](const auto& msg) {
        AINFO << "Received TypeCoverage int32=" << msg->int32_value()
              << " note=" << msg->note()
              << " py_bytes=" << ValuesToString(msg->py_bytes())
              << " py_bytearray=" << ValuesToString(msg->py_bytearray())
              << " py_memoryview=" << ValuesToString(msg->py_memoryview())
              << " fixed_bytes=" << ValuesToString(msg->fixed_bytes())
              << " samples=" << msg->samples().size()
              << " detail=(" << msg->detail().detail_id() << ","
              << msg->detail().description() << ")";
      });
  RETURN_VAL_IF(!reader, EXIT_FAILURE);
  AINFO << "Waiting for TypeCoverage messages...";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
