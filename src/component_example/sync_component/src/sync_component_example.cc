/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include "sync_component_example.h"

bool SyncComponentExample::Init() {
  AINFO << "SyncComponentExample init";
  return true;
}

bool SyncComponentExample::Proc(const std::shared_ptr<Image>& msg0,
                                const std::shared_ptr<Image>& msg1,
                                const std::shared_ptr<String>& msg2) {
  uint32_t w0 = 0, w1 = 0;
  if (msg0) w0 = msg0->width();
  if (msg1) w1 = msg1->width();
  AINFO << "SyncComponentExample Proc [image_front->width:" << w0
        << "] [image_rear->width:" << w1 << "] [chatter:" << (msg2 ? msg2->data() : "") << "]";
  return true;
}

uint64_t SyncComponentExample::GetTimeStamp(size_t index,
                                            const std::shared_ptr<Image>& msg0,
                                            const std::shared_ptr<Image>& msg1,
                                            const std::shared_ptr<String>& msg2) {
  if (index == 0 && msg0) {
    return msg0->timestamp_ms();
  }
  if (index == 1 && msg1) {
    return msg1->timestamp_ms();
  }
  if (index == 2 && msg2) {
    return msg2->timestamp_ms();
  }
  AINFO << "GetTimeStamp wrong index or null msg: " << index;
  return 0;
}

void SyncComponentExample::TimeoutProc() {
  AINFO << "SyncComponentExample TimeoutProc";
}
