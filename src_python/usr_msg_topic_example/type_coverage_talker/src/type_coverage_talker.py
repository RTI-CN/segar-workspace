#!/usr/bin/env python3

import time

from segar.python import segar
from test_msgs.msg import TypeCoverage, TypeCoverageDetail


TOPIC = "/topic/usr_msg/type_coverage"


def main():
    if not segar.Init("py_usr_msg_type_coverage_talker"):
        return 1
    node = segar.CreateNode("py_usr_msg_type_coverage_talker")
    if node is None:
        return 1
    writer = node.CreateWriter(TOPIC, TypeCoverage)
    if writer is None:
        print("[py_usr_msg_type_coverage_talker] create writer failed")
        return 1

    seq = 1
    while segar.OK():
        msg = TypeCoverage()
        msg.ready = seq % 2 == 0
        msg.int8_value = seq
        msg.uint8_value = seq
        msg.int16_value = seq
        msg.uint16_value = seq
        msg.int32_value = seq
        msg.uint32_value = seq
        msg.int64_value = seq
        msg.uint64_value = int(time.time_ns())
        msg.temperature = 25.0 + seq * 0.1
        msg.voltage = 3.3 + seq * 0.05
        msg.note = f"sample-{seq}"
        msg.raw_bytes = bytes([0xA0, 0xB0, 0xC0])
        msg.py_bytes = bytes([0xA0, 0xB0, 0xC0])
        msg.py_bytearray = bytearray([0xFB, 0xFC, 0xFD])
        msg.py_memoryview = memoryview(bytearray([0xD0, 0xD1, 0xD2]))
        msg.fixed_bytes = bytes([1, 2, 3])
        msg.samples = [seq * 10 + i for i in range(12)]
        msg.detail = TypeCoverageDetail(detail_id=seq, description=f"detail-{seq}")
        if writer.Write(msg):
            print(
                "[py_usr_msg_type_coverage_talker] sent: "
                f"int32={msg.int32_value}, py_memoryview={list(msg.py_memoryview)}"
            )
        else:
            print("[py_usr_msg_type_coverage_talker] write failed")
        seq += 1
        time.sleep(1.0)

    segar.WaitForShutdown()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
