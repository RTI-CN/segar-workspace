/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_k.h"

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeKComponent::Init() {
  node_k_writer_ = node_->CreateWriter<IntegrationTestData>("NodeK");
  sequence_num_ = 0;
  AINFO << "NodeKComponent initialized";
  return true;
}

bool NodeKComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_j_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_j_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeK input=NodeJ seq="
        << node_j_msg->sequence_number() << " latency_ns=" << latency_ns;

  AINFO << "NodeK received message :::" << node_j_msg->sequence_number();

  cpu_burner_ms(25);

  const size_t data_size = 32 * 1024;  // 32KB
  auto node_k_msg = std::make_shared<IntegrationTestData>();
  node_k_msg->sensor_id(0);
  node_k_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_k_msg->sequence_number(sequence_num_++);
  node_k_msg->data_size(static_cast<uint32_t>(data_size));
  node_k_msg->data().resize(data_size);

  for (size_t i = 0; i < data_size; ++i) {
    node_k_msg->data()[i] = static_cast<unsigned char>(i % 256);
  }

  node_k_writer_->Write(node_k_msg);
  AINFO << "NodeK published message #" << sequence_num_;

  return true;
}
