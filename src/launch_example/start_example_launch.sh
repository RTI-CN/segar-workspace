#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
OUT="$(cd "${SCRIPT_DIR}/.." && pwd)"
LAUNCH_FILE="${SCRIPT_DIR}/example.launch"
LAUNCHER="${SCRIPT_DIR}/segar_launch.py"

case "${1:-start}" in
  start)
    source "${OUT}/segar_setup.bash"
    python3 "${LAUNCHER}" start "${LAUNCH_FILE}"
    ;;
  stop)
    source "${OUT}/segar_setup.bash"
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