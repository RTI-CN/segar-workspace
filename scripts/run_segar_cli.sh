#!/usr/bin/env bash
# Iterate and run segar_cli subcommands
# Usage: ./scripts/run_segar_cli.sh [output_dir]
# Start run_all.sh or related demos first; some commands depend on live nodes/topics/services/actions

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Write each run to a dedicated log file
RUN_TS=$(date +%Y%m%d_%H%M%S)
LOG_DIR="${SCRIPT_DIR}/logs"
LOG_FILE="${LOG_DIR}/run_segar_cli_${RUN_TS}.log"
mkdir -p "$LOG_DIR"
echo "Log file: $LOG_FILE" | tee "$LOG_FILE"
echo "" | tee -a "$LOG_FILE"

source $SCRIPT_DIR/segar_setup.bash

run_cmd() {
  { echo ">>> $*"; "$@" 2>&1 || true; echo ""; } | tee -a "$LOG_FILE"
  sleep 1
}

echo "=== segar param ===" | tee -a "$LOG_FILE"
run_cmd segar param list param_server
run_cmd segar param set param_server p1_int 110
run_cmd segar param get param_server p1_int

echo "=== segar node ===" | tee -a "$LOG_FILE"
run_cmd segar node list
run_cmd segar node info common_component_example
run_cmd segar node info set_camera_info_server
run_cmd segar node info action_server

echo "=== segar topic ===" | tee -a "$LOG_FILE"
run_cmd segar topic list
run_cmd segar topic info /topic/chatter
run_cmd segar topic type /topic/chatter
run_cmd timeout 10 segar topic hz /topic/chatter
run_cmd timeout 10 segar topic bw /topic/chatter
run_cmd timeout 10 segar topic echo /topic/chatter

echo "=== segar service ===" | tee -a "$LOG_FILE"
run_cmd segar service list
run_cmd segar service type  set_camera_info
run_cmd segar service info set_camera_info

echo "=== segar action ===" | tee -a "$LOG_FILE"
run_cmd segar action list 
run_cmd segar action info lookup_transform
run_cmd segar action type lookup_transform

echo "=== segar bag (skip record, play) ===" | tee -a "$LOG_FILE"
run_cmd timeout 10 segar bag record -w /topic/chatter
run_cmd segar bag info *.record
run_cmd segar bag play -f *.record
run_cmd rm -f *.record
