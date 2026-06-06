/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <string>

#include "geometry_msgs/msg/TransformStamped.hpp"
#include "segar/segar.h"
#include "segar/time/time.h"
#include "segar/transform/static_transform_broadcaster.h"
#include "segar/transform/time_conversion.h"

namespace {

constexpr const char* kFrameMap = "map";
constexpr const char* kFrameCamera = "camera_link";

geometry_msgs::msg::TransformStamped MakeStaticMapToCamera() {
  geometry_msgs::msg::TransformStamped st;
  st.header().frame_id(kFrameMap);
  rti::segar::transform::ToHeader(
      rti::segar::transform::ToTimePoint(rti::segar::Time::Now()),
      st.header().stamp());
  st.child_frame_id(kFrameCamera);
  st.transform().translation().x(1.0);
  st.transform().translation().y(0.2);
  st.transform().translation().z(0.8);
  st.transform().rotation().x(0.0);
  st.transform().rotation().y(0.0);
  st.transform().rotation().z(0.0);
  st.transform().rotation().w(1.0);
  return st;
}

}  // namespace

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("transform_static_broadcaster");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  rti::segar::transform::StaticTransformBroadcaster broadcaster(node);
  broadcaster.SendTransform(MakeStaticMapToCamera());

  AINFO << "Published static " << kFrameMap << " -> " << kFrameCamera
        << " on " << rti::segar::transform::StaticTransformBroadcaster::kDefaultTopic;

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}

