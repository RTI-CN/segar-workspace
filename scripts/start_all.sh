#!/usr/bin/env bash
# Start all demo processes in background following README order (start only; no teardown)
# Usage: ./scripts/start_all.sh [output_dir]
# stop_all/check_all locate PIDs by process name and do not read/write PID files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# output directory precedence: arg > env var > auto-detect > default
if [ -n "${1:-}" ]; then
  OUTPUT_DIR="$1"
elif [ -n "${OUTPUT_DIR:-}" ]; then
  :
else
  if [ -d "$SCRIPT_DIR/topic_example" ]; then
    OUTPUT_DIR="$SCRIPT_DIR"
  elif [ -d "$SCRIPT_DIR/../topic_example" ]; then
    OUTPUT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
  else
    OUTPUT_DIR="$PROJ_ROOT/build_x86/output"
  fi
fi
if [ ! -d "$OUTPUT_DIR" ]; then
  echo "Error: output dir not found: $OUTPUT_DIR"
  exit 1
fi

RUN_TS=$(date +%Y%m%d_%H%M%S)
LOG_DIR="${SCRIPT_DIR}/logs/run_${RUN_TS}"
mkdir -p "$LOG_DIR"

LAUNCH_LIST=(
  "topic_example/topic_talker/scripts/launch.sh"
  "topic_example/topic_listener/scripts/launch.sh"
  "service_example/service_server/scripts/launch.sh"
  "service_example/service_client_sync/scripts/launch.sh"
  "service_example/service_client_async/scripts/launch.sh"
  "param_example/param_server/scripts/launch.sh"
  "param_example/param_client/scripts/launch.sh"
  "action_example/action_server/scripts/launch.sh"
  "action_example/action_client_sync/scripts/launch.sh"
  "action_example/action_client_async/scripts/launch.sh"
  "component_example/timer_component/scripts/launch.sh"
  "component_example/common_component/scripts/launch.sh"
  "concurrent_example/tasker/scripts/launch.sh"
)

PIDS=()
NAMES=()

run_one() {
  local rel="$1"
  local name
  name=$(echo "$rel" | sed 's|/scripts/launch.sh||')
  local workdir="$OUTPUT_DIR/$name"
  if [ ! -f "$workdir/scripts/launch.sh" ]; then
    echo "Skip (not found): $name"
    return
  fi
  local logfile="$LOG_DIR/run_all_${name//\//_}.log"
  echo "Starting: $name (log: $logfile)"
  (cd "$workdir" && exec ./scripts/launch.sh) > "$logfile" 2>&1 &
  PIDS+=( $! )
  NAMES+=( "$name" )
  sleep 0.3
}

echo "Output dir: $OUTPUT_DIR"
echo "Log dir: $LOG_DIR"
echo "---"

for rel in "${LAUNCH_LIST[@]}"; do
  run_one "$rel"
done

echo "---"
echo "All started. PIDs: ${PIDS[*]}"
echo "Logs: $LOG_DIR"
echo "To stop: ./stop_all.sh"
echo "To check: ./check_all.sh"
