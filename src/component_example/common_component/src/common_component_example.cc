/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include "common_component_example.h"

bool CommonComponentExample::Init() {
  AINFO << "CommonComponentExample init";
  return true;
}

bool CommonComponentExample::Proc(const std::shared_ptr<Image>& msg0,
                                  const std::shared_ptr<String>& msg1) {
  AINFO << "Start common component Proc [msg0->width:" << msg0->width()
        << "] [msg1->data:" << msg1->data() << "]";
  return true;
}
