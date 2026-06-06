/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <cmath>
#include <memory>
#include <string>

#include "geometry_msgs/msg/TransformStamped.hpp"
#include "segar/segar.h"
#include "segar/time/time.h"
#include "segar/transform/time_conversion.h"
#include "segar/transform/transform_broadcaster.h"

namespace {

constexpr const char* kFrameWorld = "world";
constexpr const char* kFrameBase = "base_link";

geometry_msgs::msg::TransformStamped MakeWorldToBase(double tx) {
  geometry_msgs::msg::TransformStamped st;
  st.header().frame_id(kFrameWorld);
  rti::segar::transform::ToHeader(
      rti::segar::transform::ToTimePoint(rti::segar::Time::Now()),
      st.header().stamp());
  st.child_frame_id(kFrameBase);
  st.transform().translation().x(tx);
  st.transform().translation().y(0.0);
  st.transform().translation().z(0.0);
  st.transform().rotation().x(0.0);
  st.transform().rotation().y(0.0);
  st.transform().rotation().z(0.0);
  st.transform().rotation().w(1.0);
  return st;
}

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("transform_broadcaster");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  // segar-transform 的动态 topic 固定为 rt/tf：
  // 使用 TransformBroadcaster(node) 进行动态发布。
  rti::segar::transform::TransformBroadcaster broadcaster(node);

  double phase = 0.0;
  auto callback = [&broadcaster, &phase]() {
    const double tx = std::sin(phase);
    phase += 0.05;
    broadcaster.sendTransform(MakeWorldToBase(tx));
    AINFO << "Published " << kFrameWorld << " -> " << kFrameBase
          << " translation.x=" << tx;
  };

  auto timer = std::make_shared<rti::segar::Timer>(200, callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
