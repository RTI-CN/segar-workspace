#!/usr/bin/env python3

from example.srv import SetCameraInfo
from segar.python import segar


def main():
    if not segar.Init("set_camera_info_server"):
        return 1
    node = segar.CreateNode("set_camera_info_server")
    if node is None:
        return 1

    def on_request(request):
        print(f"[service_server] request width={request.camera_info.width}")
        return SetCameraInfo.Response(
            success=True,
            status_message="Camera info set successfully",
        )

    service = node.CreateService("set_camera_info", SetCameraInfo, on_request)
    if service is None:
        print("[service_server] create service failed")
        return 1

    print("[service_server] waiting for requests...")
    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
