#!/usr/bin/env python3

import time

from example.action import LookUpTransform
from segar.python import segar


def main():
    if not segar.Init("action_client_async"):
        return 1
    node = segar.CreateNode("action_client_async")
    if node is None:
        return 1

    callbacks = segar.ActionClientCallbacks()

    def on_feedback(_client, goal_id, feedback):
        print(f"[action_client_async] feedback goal={goal_id.hex()} current={feedback.current}")

    def on_result(_client, goal_id, result, status):
        print(
            f"[action_client_async] result goal={goal_id.hex()} status={status} "
            f"transform={result.transform} error={result.error}"
        )

    def on_cancel(_client, goal_id, code):
        print(f"[action_client_async] cancel response goal={goal_id.hex()} code={int(code)}")

    callbacks.on_feedback = on_feedback
    callbacks.on_result = on_result
    callbacks.on_cancel = on_cancel

    client = node.CreateActionClient("lookup_transform", LookUpTransform, callbacks=callbacks)
    if client is None:
        print("[action_client_async] create action client failed")
        return 1

    if not client.WaitForServer(3.0):
        print("[action_client_async] wait for server timeout")
        return 1

    index = 0
    while segar.OK():
        index += 1
        goal = LookUpTransform.Goal(target_frame=f"map{index}")
        goal_id = []
        if not client.AsyncSendGoal(goal, goal_id):
            print(f"[action_client_async] AsyncSendGoal failed index={index}")
            time.sleep(1.0)
            continue

        gid = goal_id[0]
        print(f"[action_client_async] goal sent index={index} goal={gid.hex()}")
        if index % 5 == 0:
            time.sleep(0.03)
            if not client.AsyncCancelGoal(gid):
                print(f"[action_client_async] AsyncCancelGoal failed goal={gid.hex()}")
            else:
                print(f"[action_client_async] cancel sent goal={gid.hex()}")

        time.sleep(1.0)

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
