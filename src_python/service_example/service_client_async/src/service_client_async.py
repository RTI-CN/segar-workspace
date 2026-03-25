#!/usr/bin/env python3

import time

from example.msg import CameraInfo
from example.srv import SetCameraInfo
from segar.python import segar


def main():
    if not segar.Init("set_camera_info_client_async"):
        return 1
    node = segar.CreateNode("set_camera_info_client_async")
    if node is None:
        return 1

    client = node.CreateClient("set_camera_info", SetCameraInfo)
    if client is None:
        print("[service_client_async] create client failed")
        return 1

    index = 0
    while segar.OK():
        request = SetCameraInfo.Request(camera_info=CameraInfo(width=index))

        def on_response(response, req_idx):
            if response is not None:
                print(
                    f"[service_client_async] request width={req_idx}, "
                    f"response={response.status_message}"
                )
            else:
                print(f"[service_client_async] request failed, width={req_idx}")

        if not client.AsyncSendRequest(request, callback=on_response, args=index):
            print(f"[service_client_async] AsyncSendRequest failed, width={index}")
        else:
            print(f"[service_client_async] request width={index}")
        index += 1
        time.sleep(1.0)

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
