/******************************************************************************
 * Copyright (c) 2022-2026 Wang Shiming. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "node_m.h"

#include "stress_cpu.h"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/time/time.h"

bool NodeMComponent::Init() {
  AINFO << "NodeMComponent initialized";
  return true;
}

bool NodeMComponent::Proc(
    const std::shared_ptr<IntegrationTestData>& node_l_msg) {
  const uint64_t recv_ts = rti::segar::Time::Now().ToNanosecond();
  const uint64_t latency_ns = recv_ts - node_l_msg->timestamp();
  AINFO << "ITEST_LAT node=NodeM input=NodeL seq="
        << node_l_msg->sequence_number() << " latency_ns=" << latency_ns;

  AINFO << "NodeM received message :::" << node_l_msg->sequence_number();
  cpu_burner_ms(25);
  return true;
}
