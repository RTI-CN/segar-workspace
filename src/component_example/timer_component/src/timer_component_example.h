/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "example/msg/Image.hpp"

#include "segar/class_loader/class_loader.h"
#include "segar/component/component.h"
#include "segar/component/timer_component.h"

class TimerComponentExample : public rti::segar::TimerComponent {
 public:
  bool Init() final;
  bool Proc() final;

 private:
  using ImageWriter = rti::segar::Writer<example::msg::Image>;
  std::shared_ptr<ImageWriter> image_writer_ = nullptr;
  uint32_t proc_count_ = 0;
};
SEGAR_REGISTER_COMPONENT(TimerComponentExample)
