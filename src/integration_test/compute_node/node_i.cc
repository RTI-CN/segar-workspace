/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_i.h"

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

using rti::segar::Parameter;

bool NodeIComponent::Init() {
  node_i_writer_ = node_->CreateWriter<IntegrationTestData>("NodeI");
  sequence_num_ = 0;
  AINFO << "NodeIComponent initialized";

  return true;
}

bool NodeIComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& sensor11_msg,
    const std::shared_ptr<IntegrationTestData>& sensor12_msg,
    const std::shared_ptr<IntegrationTestData>& sensor13_msg,
    const std::shared_ptr<IntegrationTestData>& sensor14_msg,
    const std::shared_ptr<IntegrationTestData>& sensor15_msg,
    const std::shared_ptr<IntegrationTestData>& sensor5_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - sensor11_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeI input=sensor_topic_11 seq="
        << sensor11_msg->sequence_number() << " latency_ns=" << latency_ns;

  AINFO << "NodeI received messages :::" << sensor11_msg->sequence_number()
        << ", " << sensor12_msg->sequence_number() << ", "
        << sensor13_msg->sequence_number() << ", "
        << sensor14_msg->sequence_number() << ", "
        << sensor15_msg->sequence_number() << ", "
        << sensor5_msg->sequence_number();
  cpu_burner_ms(25);

  const size_t data_size = 256 * 1024;  // 256KB
  auto node_i_msg = std::make_shared<IntegrationTestData>();
  node_i_msg->sensor_id(0);
  node_i_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_i_msg->sequence_number(sequence_num_++);
  node_i_msg->data_size(static_cast<uint32_t>(data_size));
  node_i_msg->data().resize(data_size);

  for (size_t i = 0; i < data_size; ++i) {
    node_i_msg->data()[i] = static_cast<unsigned char>(i % 256);
  }

  node_i_writer_->Write(node_i_msg);
  AINFO << "NodeI published message #" << sequence_num_;

  return true;
}
