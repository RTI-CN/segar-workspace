#!/usr/bin/env python3

import time

from segar.python import parameter
from segar.python import segar


def main():
    if not segar.Init("param_client"):
        return 1
    node = segar.CreateNode("param_client")
    if node is None:
        return 1

    service_node = "param_server"

    while segar.OK():
        out_int = parameter.OutValue()
        out_str = parameter.OutValue()

        if not parameter.Segar_Get_Remote_Param(service_node, "p1_int", out_int, int):
            print("[param_client] waiting p1_int...")
            time.sleep(1.0)
            continue
        if not parameter.Segar_Get_Remote_Param(service_node, "p2_string", out_str, str):
            print("[param_client] waiting p2_string...")
            time.sleep(1.0)
            continue

        print(f"[param_client] p1_int={out_int.value} p2_string={out_str.value}")

        if not parameter.Segar_Load_Remote_Params(service_node, "/tmp/param_server.params"):
            print("[param_client] load remote params failed")
            time.sleep(1.0)
            continue

        params = []
        if parameter.Segar_List_Remote_Params(service_node, params):
            print(f"[param_client] remote params={len(params)}")
            for p in params:
                print(f"[param_client] {p.debug_string()}")

        parameter.Segar_Dump_Remote_Params(service_node, "/tmp/param_server.params")
        break

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
