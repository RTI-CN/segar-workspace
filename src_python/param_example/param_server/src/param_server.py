#!/usr/bin/env python3

from segar.python import parameter
from segar.python import segar


def main():
    if not segar.Init("param_server"):
        return 1
    node = segar.CreateNode("param_server")
    if node is None:
        return 1

    if not parameter.Segar_Load_Local_Params(node, "config/params.yaml"):
        print("[param_server] load local params failed")
        return 1

    if not parameter.Segar_Set_Local_Param(node, "p1_int", 1):
        return 1
    if not parameter.Segar_Set_Local_Param(node, "p2_string", "test"):
        return 1

    params = []
    if not parameter.Segar_List_Local_Params(node, params):
        return 1
    print(f"[param_server] local params={len(params)}")
    for p in params:
        print(f"[param_server] {p.debug_string()}")

    if not parameter.Segar_Dump_Local_Params(node, "/tmp/param_server.params"):
        return 1
    print("[param_server] dumped to /tmp/param_server.params")

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
