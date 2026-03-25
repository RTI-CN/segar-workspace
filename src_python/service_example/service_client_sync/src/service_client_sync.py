#!/usr/bin/env python3

import time

from example.msg import CameraInfo
from example.srv import SetCameraInfo
from segar.python import segar


def main():
    if not segar.Init("set_camera_info_client_sync"):
        return 1
    node = segar.CreateNode("set_camera_info_client_sync")
    if node is None:
        return 1

    client = node.CreateClient("set_camera_info", SetCameraInfo)
    if client is None:
        print("[service_client_sync] create client failed")
        return 1

    index = 0
    while segar.OK():
        request = SetCameraInfo.Request(camera_info=CameraInfo(width=index))
        response = client.SyncSendRequest(request)
        if response is not None:
            print(
                f"[service_client_sync] request width={index}, "
                f"response={response.status_message}"
            )
        else:
            print(f"[service_client_sync] request failed, width={index}")
        index += 1
        time.sleep(1.0)

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
