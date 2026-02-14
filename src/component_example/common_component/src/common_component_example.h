/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "example/msg/Image.hpp"
#include "example/msg/String.hpp"

#include "segar/component/component.h"
using example::msg::Image;
using example::msg::String;

class CommonComponentExample : public rti::segar::Component<Image, String> {
 public:
  bool Init() final;
  bool Proc(const std::shared_ptr<Image>& msg0,
            const std::shared_ptr<String>& msg1) final;
};
SEGAR_REGISTER_COMPONENT(CommonComponentExample)
