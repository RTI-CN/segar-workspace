/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include "example/msg/Image.hpp"
#include "example/msg/String.hpp"

#include "segar/class_loader/class_loader.h"
#include "segar/component/sync_component.h"

using example::msg::Image;
using example::msg::String;

class SyncComponentExample : public rti::segar::SyncComponent<Image, Image, String> {
 public:
  bool Init() final;
  bool Proc(const std::shared_ptr<Image>& msg0,
            const std::shared_ptr<Image>& msg1,
            const std::shared_ptr<String>& msg2) final;
  void TimeoutProc() final;
  uint64_t GetTimeStamp(size_t index,
                        const std::shared_ptr<Image>& msg0,
                        const std::shared_ptr<Image>& msg1,
                        const std::shared_ptr<String>& msg2) final;
};
SEGAR_REGISTER_COMPONENT(SyncComponentExample)
