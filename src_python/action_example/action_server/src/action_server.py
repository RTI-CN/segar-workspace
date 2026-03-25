#!/usr/bin/env python3

import time

from example.action import LookUpTransform
from segar.python import segar


def main():
    if not segar.Init("action_server"):
        return 1
    node = segar.CreateNode("action_server")
    if node is None:
        return 1

    def on_goal(_server, goal_id, goal):
        print(f"[action_server] accept goal={goal_id.hex()} target={goal.target_frame}")
        return True

    def on_cancel(_server, goal_id):
        print(f"[action_server] cancel request goal={goal_id.hex()}")
        return True

    def on_execute(server, goal_id, goal):
        steps = 5
        for i in range(1, steps + 1):
            server.PublishFeedback(goal_id, LookUpTransform.Feedback(current=i))
            if server.IsCancelRequested(goal_id):
                server.CancelGoal(
                    goal_id,
                    LookUpTransform.Result(transform="cancelled", error=-1),
                )
                print(f"[action_server] canceled goal={goal_id.hex()} at step={i}")
                return
            time.sleep(0.02)

        result = LookUpTransform.Result(
            transform=f"transform_from_{goal.target_frame}",
            error=0,
        )
        server.Succeed(goal_id, result)
        print(f"[action_server] finished goal={goal_id.hex()}")

    callbacks = segar.ActionServerCallbacks(
        on_goal=on_goal,
        on_cancel=on_cancel,
        on_execute=on_execute,
    )
    server = node.CreateActionServer("lookup_transform", LookUpTransform, callbacks=callbacks)
    if server is None:
        print("[action_server] create action server failed")
        return 1

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
