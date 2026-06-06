/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>
#include <string>

#include "segar/segar.h"
#include "segar/transform/buffer.h"
#include "tf2/exceptions.hpp"

namespace {

constexpr const char* kFrameWorld = "world";
constexpr const char* kFrameBase = "base_link";

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  rti::segar::transform::Buffer buffer;
  RETURN_VAL_IF(!buffer.Init(), EXIT_FAILURE);

  auto callback = [&buffer]() {
    std::string err;
    const rti::segar::Time latest;
    if (!buffer.canTransform(kFrameBase, kFrameWorld, latest, 0.5f, &err)) {
      AINFO << "Waiting for transform " << kFrameWorld << " -> " << kFrameBase
            << ": " << err;
      return;
    }
    try {
      const auto tr = buffer.lookupTransform(kFrameBase, kFrameWorld, latest);
      const auto& t = tr.transform().translation();
      AINFO << "lookupTransform(" << kFrameBase << "," << kFrameWorld
            << "): translation=(" << t.x() << ", " << t.y() << ", " << t.z()
            << ")";
    } catch (const tf2::TransformException& ex) {
      AWARN << "lookupTransform failed: " << ex.what();
    }
  };

  auto timer = std::make_shared<rti::segar::Timer>(500, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();
  buffer.Shutdown();
  return EXIT_SUCCESS;
}
