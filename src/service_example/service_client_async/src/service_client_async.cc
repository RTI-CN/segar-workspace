/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>

#include "example/srv/SetCameraInfo.hpp"

#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("set_camera_info_client_async");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using SetCameraInfo = example::srv::SetCameraInfo;
  auto client = node->CreateClient<SetCameraInfo>("set_camera_info");
  RETURN_VAL_IF(!client, EXIT_FAILURE);

  uint32_t index = 0;
  auto timer_callback = [&client, &index]() {
    auto request = std::make_shared<SetCameraInfo::Request>();
    request->camera_info().width(index);

    auto response_callback =
        [index](const std::shared_ptr<SetCameraInfo::Response>& response) {
          if (response) {
            AINFO << "[Async] request camera width: " << index
                  << ", Response msg:" << response->status_message();
          } else {
            AINFO << "[Async] Request failed, request camera width: " << index;
          }
        };
    client->AsyncSendRequest(request, response_callback);
    AINFO << "[Async] request camera width: " << index;
    index++;
  };

  // 1hz
  auto timer = std::make_shared<rti::segar::Timer>(1000, timer_callback, false);
  timer->Start();
  rti::segar::WaitForShutdown();

  return EXIT_SUCCESS;
}
