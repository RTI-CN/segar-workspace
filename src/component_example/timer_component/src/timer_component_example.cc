/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include "segar/time/clock.h"

#include "timer_component_example.h"

bool TimerComponentExample::Init() {
  image_front_writer_ =
      node_->CreateWriter<example::msg::Image>("/topic/image_front");
  RETURN_VAL_IF(!image_front_writer_, false);
  image_rear_writer_ =
      node_->CreateWriter<example::msg::Image>("/topic/image_rear");
  RETURN_VAL_IF(!image_rear_writer_, false);
  return true;
}

bool TimerComponentExample::Proc() {
  auto out_msg = std::make_shared<example::msg::Image>();
  out_msg->timestamp_ms(rti::segar::Clock::Now().ToMillisecond());
  out_msg->width(proc_count_++);
  AINFO_IF(!image_front_writer_->Write(out_msg))
      << "Failed to write image_front msg:" << out_msg->width();
  AINFO_IF(!image_rear_writer_->Write(out_msg))
      << "Failed to write image_rear msg:" << out_msg->width();
  AINFO << "timer_component_example: Write image_front/image_rear msg->width:"
        << out_msg->width();
  return true;
}
