#!/usr/bin/env python3

import time

from example.action import LookUpTransform
from segar.python import segar


def _send_sync_goal(client, index):
    goal = LookUpTransform.Goal(target_frame=f"map{index}")
    goal_id = []
    if not client.SyncSendGoal(goal, goal_id):
        print(f"[action_client_sync] SyncSendGoal failed index={index}")
        return
    gid = goal_id[0]
    result = client.WaitForResult(gid)
    if result is None:
        print(f"[action_client_sync] WaitForResult failed goal={gid.hex()}")
        return
    print(
        f"[action_client_sync] result goal={gid.hex()} status={result.status} "
        f"transform={result.result.transform} error={result.result.error}"
    )


def _send_and_cancel(client, index):
    goal = LookUpTransform.Goal(target_frame=f"map{index}")
    goal_id = []
    if not client.SyncSendGoal(goal, goal_id):
        print(f"[action_client_sync] SyncSendGoal failed index={index}")
        return
    gid = goal_id[0]
    time.sleep(0.05)
    if not client.SyncCancelGoal(gid):
        print(f"[action_client_sync] SyncCancelGoal failed goal={gid.hex()}")
        return
    print(f"[action_client_sync] cancel sent goal={gid.hex()}")
    result = client.WaitForResult(gid)
    if result is None:
        print(f"[action_client_sync] WaitForResult failed goal={gid.hex()}")
        return
    print(
        f"[action_client_sync] cancel result goal={gid.hex()} "
        f"status={result.status} transform={result.result.transform}"
    )


def main():
    if not segar.Init("action_client_sync"):
        return 1
    node = segar.CreateNode("action_client_sync")
    if node is None:
        return 1

    client = node.CreateActionClient("lookup_transform", LookUpTransform)
    if client is None:
        print("[action_client_sync] create action client failed")
        return 1

    if not client.WaitForServer(3.0):
        print("[action_client_sync] wait for server timeout")
        return 1

    index = 0
    while segar.OK():
        index += 1
        if index % 2 == 0:
            _send_and_cancel(client, index)
        else:
            _send_sync_goal(client, index)
        time.sleep(1.0)

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
