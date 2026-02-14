/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "timer_component_example.h"

bool TimerComponentExample::Init() {
  image_writer_ = node_->CreateWriter<example::msg::Image>("/topic/image");
  RETURN_VAL_IF(!image_writer_, false);
  return true;
}

bool TimerComponentExample::Proc() {
  auto out_msg = std::make_shared<example::msg::Image>();
  out_msg->width(proc_count_++);
  AINFO_IF(!image_writer_->Write(out_msg))
      << "Failed to write msg:" << out_msg->width();
  AINFO << "timer_component_example: Write image msg->width:"
        << out_msg->width();
  return true;
}
