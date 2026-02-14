#!/usr/bin/env bash
set -euo pipefail

ROOT="/home/simon/segar/Segar2"
OUT="${ROOT}/build_x86/output"
LAUNCH_FILE="${ROOT}/temp.launch"
LAUNCHER="${ROOT}/segar/tools/segar_launch/segar_launch.py"

case "${1:-start}" in
  start)
    source "${OUT}/setup.bash"
    python3 "${LAUNCHER}" start "${LAUNCH_FILE}"
    ;;
  stop)
    source "${OUT}/setup.bash"
    python3 "${LAUNCHER}" stop "${LAUNCH_FILE}"
    ;;
  status)
    echo "mainboard: $(pgrep -cx mainboard || true)"
    echo "sensor_node: $(pgrep -cx sensor_node || true)"
    pgrep -af "segar_launch.py start ${LAUNCH_FILE}" || true
    ;;
  *)
    echo "Usage: $0 {start|stop|status}"
    exit 1
    ;;
esac