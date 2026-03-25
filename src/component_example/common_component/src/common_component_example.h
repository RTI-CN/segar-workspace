/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR
 * SPDX-License-Identifier: Apache-2.0
 *****************************************************************************/

#include "example/msg/Image.hpp"
#include "example/msg/String.hpp"

#include "segar/component/component.h"
using example::msg::Image;
using example::msg::String;

class CommonComponentExample : public rti::segar::Component<String, Image, Image> {
 public:
  bool Init() final;
  bool Proc(const std::shared_ptr<String>& msg0,
            const std::shared_ptr<Image>& msg1,
            const std::shared_ptr<Image>& msg2) final;
};
SEGAR_REGISTER_COMPONENT(CommonComponentExample)
