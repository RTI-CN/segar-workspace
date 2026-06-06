#!/usr/bin/env bash
set -eo pipefail

# Script runs under integration_test/tools/; setup.bash is in integration_test's parent dir
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SETUP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

SETUP_SCRIPT="${SETUP_DIR}/segar_setup.bash"

DEFAULT_BASE_DIR="$(cd "${SCRIPT_DIR}" && pwd)"

# Source setup.bash to set environment variables
if [ -f "${SETUP_SCRIPT}" ]; then
  ORIGINAL_DIR=$(pwd)
  cd "$(dirname "${SETUP_SCRIPT}")" || exit 1
  # shellcheck disable=SC1090
  source "${SETUP_SCRIPT}"
  cd "${ORIGINAL_DIR}" || exit 1
else
  echo "Warning: setup.bash not found at ${SETUP_SCRIPT}"
  echo "Some environment variables may not be set correctly."
fi

BASE_DIR="${1:-${DEFAULT_BASE_DIR}}"
if [ ! -d "$BASE_DIR" ]; then
  echo "$BASE_DIR: No such file or directory"
  echo "Need to set SEGAR_INTEGRATION_TEST=ON"
  exit 1
fi
BASE_DIR="$(cd "$BASE_DIR" && pwd)"

INTEGRATION_DIR="$BASE_DIR"
cd "$INTEGRATION_DIR" || exit 1

# Explicitly pin both local and global config roots to the installed output
# tree so integration_test does not depend on whatever the parent shell
# happened to export before running this script.
OUTPUT_ROOT="$(cd "${INTEGRATION_DIR}/.." && pwd)"
export SEGAR_PATH="$OUTPUT_ROOT"
export SEGAR_GLOBAL_PATH="$OUTPUT_ROOT"

# Ensure runtime can resolve framework and integration test libraries.
export LD_LIBRARY_PATH="$INTEGRATION_DIR/lib:$OUTPUT_ROOT/lib:$OUTPUT_ROOT/third_party/lib:${LD_LIBRARY_PATH:-}"
export SEGAR_MSG_LIB_PATH="$OUTPUT_ROOT/msg_libs:$OUTPUT_ROOT/third_party/msg_libs:${SEGAR_MSG_LIB_PATH:-}"

# Validate required paths before creating LOG_DIR (fail fast)
if [ ! -d "$INTEGRATION_DIR/dag" ]; then
  echo "Error: $INTEGRATION_DIR/dag not found"
  exit 1
fi
SENSOR_BIN_DIR="$INTEGRATION_DIR/bin"
SENSOR_CONFIG_FILE="$INTEGRATION_DIR/params/sensor_config.json"
if [ ! -f "$SENSOR_CONFIG_FILE" ]; then
  echo "Error: Config file not found: $SENSOR_CONFIG_FILE"
  exit 1
fi
if [ ! -x "$SENSOR_BIN_DIR/sensor_node" ]; then
  echo "Error: Executable not found: $SENSOR_BIN_DIR/sensor_node"
  exit 1
fi

LOG_DIR="${2:-$INTEGRATION_DIR/logs/itest_$(date +%Y%m%d_%H%M%S)}"
DURATION_SEC="${3:-60}"
PROGRESS_INTERVAL_SEC="${PROGRESS_INTERVAL_SEC:-3}"
NPROC="$(nproc 2>/dev/null || echo 1)"
EXIT_REASON_FILE="$LOG_DIR/exit_reason.txt"
PARSE_SCRIPT="$INTEGRATION_DIR/parse_itest_logs.py"

mkdir -p "$LOG_DIR"
if ! [ -w "$LOG_DIR" ]; then
  echo "Error: LOG_DIR not writable: $LOG_DIR"
  exit 1
fi

pids=()
pid_starttimes=()
CLEANED=0
USE_SETSID=0

# Get process start time from /proc for PID identity check (avoid mistaking reused PID as our child)
get_proc_starttime() {
  [ -r "/proc/$1/stat" ] && awk '{print $22}' "/proc/$1/stat" 2>/dev/null
}

if ! command -v mainboard >/dev/null 2>&1; then
  echo "Error: mainboard not found; executable is not in PATH"
  exit 1
fi

if command -v setsid >/dev/null 2>&1; then
  USE_SETSID=1
fi

# Process names that this test may start; used for pre/post leftover cleanup and check
INT_LEFTOVER_NAMES=(mainboard sensor_node resource_monitor)

# Kill processes by name (handles orphaned processes and PID tracking failures).
# Use -f as well as -x so processes started via "bash -c ..." (comm=bash) are matched.
kill_processes_by_name() {
  local signal="$1"
  pkill "$signal" -x "mainboard" 2>/dev/null || true
  pkill "$signal" -f "mainboard" 2>/dev/null || true
  pkill "$signal" -x "sensor_node" 2>/dev/null || true
  pkill "$signal" -f "sensor_node" 2>/dev/null || true
  pkill "$signal" -f "start_sensor.sh" 2>/dev/null || true
  pkill "$signal" -f "bash.*start_sensor" 2>/dev/null || true
  pkill "$signal" -f "resource_monitor" 2>/dev/null || true
  pkill "$signal" -f "segar.*resource_monitor" 2>/dev/null || true
  pkill "$signal" -f "sh -c.*resource_monitor" 2>/dev/null || true
  pkill "$signal" -x "resource_monitor" 2>/dev/null || true
}

# Check if any leftover test processes are still running; exit 1 and prompt manual kill if so.
# Use -f as well as -x so processes started via "bash -c ..." are detected.
check_leftover() {
  local name
  for name in "${INT_LEFTOVER_NAMES[@]}"; do
    if pgrep -x "$name" >/dev/null 2>&1 || pgrep -f "$name" >/dev/null 2>&1; then
      echo "ERROR: Leftover process(es) still running: $name. Please kill them manually (e.g. pkill -KILL -f $name) and retry." >&2
      exit 1
    fi
  done
}

# Kill leftover processes with INT only (no KILL), then verify cleanup; exit 1 if still leftover
cleanup_leftover_int_only() {
  kill_processes_by_name "-INT"
  sleep 2
  check_leftover
}

# Get current system CPU usage string: e.g. "50%(400/800)" for 8 cores at 50% total usage
get_current_cpu_display() {
  local stat1 stat2 total1 idle1 total2 idle2 total_delta idle_delta used_pct max_capacity
  stat1=$(grep '^cpu ' /proc/stat 2>/dev/null)
  sleep 0.5 2>/dev/null || sleep 1
  stat2=$(grep '^cpu ' /proc/stat 2>/dev/null)
  if [ -z "$stat1" ] || [ -z "$stat2" ]; then
    echo "N/A"
    return
  fi
  total1=$(echo "$stat1" | awk '{t=0; for(i=2;i<=NF;i++) t+=$i; print t}')
  idle1=$(echo "$stat1" | awk '{print $5+$6}')
  total2=$(echo "$stat2" | awk '{t=0; for(i=2;i<=NF;i++) t+=$i; print t}')
  idle2=$(echo "$stat2" | awk '{print $5+$6}')
  total_delta=$((total2 - total1))
  idle_delta=$((idle2 - idle1))
  if [ "$total_delta" -le 0 ]; then
    echo "N/A"
    return
  fi
  used_pct=$(( (total_delta - idle_delta) * 100 / total_delta ))
  max_capacity=$((NPROC * 100))
  echo "${used_pct}%($((used_pct * NPROC))/${max_capacity})"
}

echo "Cleaning pre-existing mainboard/sensor_node/resource_monitor processes (current user)"
cleanup_leftover_int_only

# Cleanup: ignore INT/TERM during run so second Ctrl+C cannot interrupt.
# Order: kill tracked PIDs first (our children), then by name (orphans); avoid parent zombie.
cleanup() {
  if [ "$CLEANED" -eq 1 ]; then
    return
  fi
  CLEANED=1
  echo "Stopping..."
  trap '' INT TERM
  exec 3>&2
  exec 2>/dev/null

  # 1) Kill tracked PIDs first (our children), then by name (orphans)
  for pid in "${pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      kill -TERM "$pid" 2>/dev/null || true
    fi
  done
  kill_processes_by_name "-TERM"
  sleep 2

  # 2) Send INT to remaining (no KILL)
  for pid in "${pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      kill -INT "$pid" 2>/dev/null || true
    fi
  done
  kill_processes_by_name "-INT"
  sleep 2
  wait || true
  exec 2>&3
  exec 3>&-
  check_leftover
}

finalize() {
  cleanup
  # Restore INT/TERM trap only after cleanup finishes (so cleanup is never interrupted)
  trap 'echo "interrupt" > "$EXIT_REASON_FILE"; exit 0' INT TERM
  if [ -f "$EXIT_REASON_FILE" ]; then
    exit_reason="$(cat "$EXIT_REASON_FILE")"
    echo "Exit reason: $exit_reason"
    end_ts="$(date +%s)"
    elapsed_sec=$((end_ts - START_TS))
    elapsed_min=$((elapsed_sec / 60))
    elapsed_rem=$((elapsed_sec % 60))
    printf "Test duration: %d min %02d sec\n" "$elapsed_min" "$elapsed_rem"
  else
    echo "Exit reason: completed"
  fi
  if [ -f "$PARSE_SCRIPT" ]; then
    python3 "$PARSE_SCRIPT" "$LOG_DIR" || true
  fi
}

# Register traps before starting processes to ensure cleanup is called
trap 'echo "interrupt" > "$EXIT_REASON_FILE"; exit 0' INT TERM
trap 'finalize' EXIT

start_bg() {
  local cmd="$1"
  local logfile="$2"
  if [ "$USE_SETSID" -eq 1 ]; then
    setsid bash -c "$cmd" >"$logfile" 2>&1 &
  else
    bash -c "$cmd" >"$logfile" 2>&1 &
  fi
  pids+=("$!")
  pid_starttimes+=("$(get_proc_starttime "$!")")
}

# Start mainboard processes
TRACING_NODE_DAG="$INTEGRATION_DIR/dag/tracing_node.dag"
if [ -f "$TRACING_NODE_DAG" ]; then
  name="$(basename "$TRACING_NODE_DAG" .dag)"
  echo "Starting DAG (first): $TRACING_NODE_DAG"
  start_bg "mainboard -d \"$TRACING_NODE_DAG\"" "$LOG_DIR/${name}.log"
fi

for dag in "$INTEGRATION_DIR"/dag/*.dag; do
  if [ "$dag" = "$TRACING_NODE_DAG" ]; then
    continue
  fi
  name="$(basename "$dag" .dag)"
  echo "Starting DAG: $dag"
  start_bg "mainboard -d \"$dag\"" "$LOG_DIR/${name}.log"
done

# Start sensor processes (SENSOR_BIN_DIR/SENSOR_CONFIG_FILE already validated above)
echo "Starting sensor processes sensor node 1...sensor node 15"
# Start each sensor_node process with its own log file (no extra quotes around executable; quote config path only)
for i in {1..15}; do
    echo "Starting sensor node $i..."
    start_bg "${SENSOR_BIN_DIR}/sensor_node "${SENSOR_CONFIG_FILE}" $i" "$LOG_DIR/sensor_node_${i}.log"
done

# Wait for processes to start
sleep 3

# Start resource monitor (use start_bg so startup failure is consistent and pid is tracked)
echo "Starting resource monitor"
resource_log="$LOG_DIR/resource.log"
monitor_pids=()
while IFS= read -r pid; do
  if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
    monitor_pids+=("$pid")
  fi
done < <(ps -eo pid,comm --no-headers | awk '$2 ~ /(mainboard|sensor_node)/ {print $1}')

if [ ${#monitor_pids[@]} -gt 0 ]; then
  echo "Monitoring process PIDs: ${monitor_pids[*]}"
  start_bg "segar resource_monitor ${monitor_pids[*]}" "$resource_log"
  last_pid=${pids[${#pids[@]}-1]}
  if ! kill -0 "$last_pid" 2>/dev/null; then
    echo "Error: resource_monitor failed to start (pid $last_pid)"
    exit 1
  fi
else
  echo "Warning: No processes found to monitor (mainboard or sensor_node)"
fi

# Start test timer only after all processes are started (excludes init time)
START_TS="$(date +%s)"
LAST_PROGRESS_TS="$START_TS"

if [ "${#pids[@]}" -eq 0 ]; then
  echo "Error: No processes started (no DAGs or start_bg failed)"
  exit 1
fi

echo "Running... Log directory: $LOG_DIR"
echo "Press Ctrl+C to stop"

# Main monitoring loop
while true; do
  if [ "${#pids[@]}" -eq 0 ]; then
    echo "completed" > "$EXIT_REASON_FILE"
    echo "All processes exited normally"
    exit 0
  fi

  # Progress feedback
  now_ts="$(date +%s)"
  if [ "$PROGRESS_INTERVAL_SEC" -gt 0 ] && [ $((now_ts - LAST_PROGRESS_TS)) -ge "$PROGRESS_INTERVAL_SEC" ]; then
    elapsed_sec=$((now_ts - START_TS))
    elapsed_min=$((elapsed_sec / 60))
    elapsed_rem=$((elapsed_sec % 60))
    cpu_display="$(get_current_cpu_display)"
    if [ "$DURATION_SEC" -gt 0 ]; then
      total_sec=$((DURATION_SEC))
      percent=$((elapsed_sec * 100 / total_sec))
      if [ "$percent" -gt 100 ]; then percent=100; fi
      printf "[progress] %d/%d sec (%d%%), running=%d, current-cpu: %s\n" "$elapsed_sec" "$total_sec" "$percent" "${#pids[@]}" "$cpu_display"
    else
      printf "[progress] %d min %02d sec, running=%d, current-cpu: %s\n" "$elapsed_min" "$elapsed_rem" "${#pids[@]}" "$cpu_display"
    fi
    LAST_PROGRESS_TS="$now_ts"
  fi

  # Check duration
  if [ "$DURATION_SEC" -gt 0 ] && [ $((now_ts - START_TS)) -ge "$DURATION_SEC" ]; then
    echo "Test finished!" > "$EXIT_REASON_FILE"
    exit 0
  fi

  # Check process status: validate PID identity via /proc starttime (avoid mistaking reused PID as our child)
  next_pids=()
  next_starttimes=()
  for idx in "${!pids[@]}"; do
    pid=${pids[$idx]}
    starttime=${pid_starttimes[$idx]}
    if ! kill -0 "$pid" 2>/dev/null; then
      wait "$pid" 2>/dev/null
      status=$?
      # Treat 0, 130 (SIGINT), 143 (SIGTERM) as expected (normal exit or graceful shutdown)
      if [ "$status" -ne 0 ] && [ "$status" -ne 130 ] && [ "$status" -ne 143 ]; then
        echo "process_exit_${pid}" > "$EXIT_REASON_FILE"
        echo "Error: process $pid exited abnormally, status $status"
        exit 1
      fi
    else
      cur=$(get_proc_starttime "$pid")
      # If no /proc (starttime empty), fall back to kill -0 only; else require starttime match (PID identity)
      if [ -z "$starttime" ] || [ "$cur" = "$starttime" ]; then
        next_pids+=("$pid")
        next_starttimes+=("$starttime")
      fi
    fi
  done
  pids=("${next_pids[@]}")
  pid_starttimes=("${next_starttimes[@]}")
  sleep 1
done
