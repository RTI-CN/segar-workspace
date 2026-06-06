/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_l.h"

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeLComponent::Init() {
  node_l_writer_ = node_->CreateWriter<IntegrationTestData>("NodeL");
  sequence_num_ = 0;
  AINFO << "NodeLComponent initialized";
  return true;
}

bool NodeLComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_f_msg,
    const std::shared_ptr<IntegrationTestData>& node_k_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_f_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeL input=NodeF seq="
        << node_f_msg->sequence_number() << " latency_ns=" << latency_ns;

  AINFO << "NodeL received messages :::" << node_f_msg->sequence_number()
        << ", " << node_k_msg->sequence_number();
  cpu_burner_ms(25);

  const size_t data_size = 16 * 1024;  // 16KB
  auto node_l_msg = std::make_shared<IntegrationTestData>();
  node_l_msg->sensor_id(0);
  node_l_msg->timestamp(rti::segar::Time::Now().ToNanosecond());
  node_l_msg->sequence_number(sequence_num_++);
  node_l_msg->data_size(static_cast<uint32_t>(data_size));
  node_l_msg->data().resize(data_size);

  for (size_t i = 0; i < data_size; ++i) {
    node_l_msg->data()[i] = static_cast<unsigned char>(i % 256);
  }

  node_l_writer_->Write(node_l_msg);
  AINFO << "NodeL published message #" << sequence_num_;

  return true;
}
