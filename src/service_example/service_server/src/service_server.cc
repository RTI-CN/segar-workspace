/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>
#include <string>

#include "example/srv/SetCameraInfo.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("set_camera_info_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using SetCameraInfo = example::srv::SetCameraInfo;
  auto callback = [](const std::shared_ptr<SetCameraInfo::Request>& request,
                     std::shared_ptr<SetCameraInfo::Response>& response) {
    response->success(true);
    response->status_message("Camera info set successfully");
    AINFO << "Request camera width: " << request->camera_info().width()
          << ", Response msg:" << response->status_message();
  };
  auto service =
      node->CreateService<SetCameraInfo>("set_camera_info", callback);
  RETURN_VAL_IF(!service, EXIT_FAILURE);

  AINFO << "Waiting for requests...";
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
