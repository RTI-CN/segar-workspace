/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_j.h"

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeJComponent::Init() {
  node_j_writer_ = node_->CreateWriter<IntegrationTestData>("NodeJ");
  sequence_num_ = 0;
  AINFO << "NodeJComponent initialized";
  return true;
}

bool NodeJComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_e_msg,
    const std::shared_ptr<IntegrationTestData>& node_i_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_e_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeJ input=NodeE seq="
        << node_e_msg->sequence_number() << " latency_ns=" << latency_ns;

  AINFO << "NodeJ received messages :::" << node_e_msg->sequence_number()
        << ", " << node_i_msg->sequence_number();
  cpu_burner_ms(25);

  const size_t data_size = 64 * 1024;  // 64KB
  auto node_j_msg = std::make_shared<IntegrationTestData>();
  node_j_msg->sensor_id(0);
  node_j_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_j_msg->sequence_number(sequence_num_++);
  node_j_msg->data_size(static_cast<uint32_t>(data_size));
  node_j_msg->data().resize(data_size);

  for (size_t i = 0; i < data_size; ++i) {
    node_j_msg->data()[i] = static_cast<unsigned char>(i % 256);
  }

  node_j_writer_->Write(node_j_msg);
  AINFO << "NodeJ published message #" << sequence_num_;

  return true;
}
