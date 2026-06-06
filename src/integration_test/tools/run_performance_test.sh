#!/usr/bin/env bash
set -eo pipefail

# Script runs under integration_test/tools/; setup.bash is in integration_test's parent dir
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SETUP_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

SETUP_SCRIPT="${SETUP_DIR}/segar_setup.bash"

DEFAULT_BASE_DIR="$(cd "${SCRIPT_DIR}" && pwd)"

TIMEOUT_DURATION=600

if [ -f "${SETUP_SCRIPT}" ]; then
  ORIGINAL_DIR="$(pwd)"
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

BIN_DIR="${INTEGRATION_DIR}/bin"
if [ ! -d "${BIN_DIR}" ]; then
  echo "Error: ${BIN_DIR} not found"
  exit 1
fi

LOG_DIR="${2:-$INTEGRATION_DIR/logs/perf_$(date +%Y%m%d_%H%M%S)}"
mkdir -p "${LOG_DIR}"

USE_SETSID=0
if command -v setsid >/dev/null 2>&1; then
  USE_SETSID=1
fi

pids=()
pg_kill=()
CLEANED=0
tracing_node_pid=""
tracing_node_pg=0
TRACING_NODE_DAG="${INTEGRATION_DIR}/dag/tracing_node.dag"
TRACING_NODE_PATTERN="tracing_node.dag"

# Store start time for timeout check
START_TIME=$(date +%s)

add_pid() {
  pids+=("$1")
  pg_kill+=("$2")
}

# Remove PID from tracking arrays after process has exited (avoids PID reuse false positives)
remove_pid() {
  local target="$1"
  local new_pids=() new_pg=()
  local i pid
  for i in "${!pids[@]}"; do
    pid="${pids[$i]}"
    if [ "$pid" != "$target" ]; then
      new_pids+=("$pid")
      new_pg+=("${pg_kill[$i]}")
    fi
  done
  pids=("${new_pids[@]}")
  pg_kill=("${new_pg[@]}")
}

# Start background process. Args: executable [arg1 arg2 ...] logfile (last arg is logfile).
# On return: REPLY_PID and REPLY_PG are set for the started process.
# Verifies the process actually started (fails fast if executable missing / permission denied).
start_bg() {
  local logfile="${*: -1}"
  local -a args=("${@:1:$#-1}")
  local bg_pid exe
  exe="${args[0]}"
  if [ "$USE_SETSID" -eq 1 ]; then
    setsid bash -c 'exec "$@"' _ "${args[@]}" >"$logfile" 2>&1 &
    bg_pid=$!
    add_pid "$bg_pid" 1
    REPLY_PG=1
  else
    bash -c 'exec "$@"' _ "${args[@]}" >"$logfile" 2>&1 &
    bg_pid=$!
    add_pid "$bg_pid" 0
    REPLY_PG=0
  fi
  REPLY_PID=$bg_pid
  sleep 0.2
  if ! kill -0 "$bg_pid" 2>/dev/null; then
    echo "ERROR: Process failed to start: $exe (PID $bg_pid exited immediately). Check $logfile for details." >&2
    remove_pid "$bg_pid"
    exit 1
  fi
}

# Process names that this test may start; used for pre/post leftover cleanup and check
PERF_LEFTOVER_NAMES=(segar_node_a segar_node_b segar_service segar_client segar_client_async segar_action_server segar_action_client_sync)

# Send signal to performance test processes by name
kill_perf_processes_by_name() {
  local signal="$1"
  local name
  for name in "${PERF_LEFTOVER_NAMES[@]}"; do
    pkill "$signal" -x "$name" 2>/dev/null || true
  done
}

# Kill tracing_node mainboard by dag pattern (avoid killing unrelated mainboard processes)
kill_tracing_node_by_cmd() {
  local signal="$1"
  pkill "$signal" -f "$TRACING_NODE_PATTERN" 2>/dev/null || true
}

# Check if any process matching PERF_LEFTOVER_NAMES is still running (by name). Used for pre-test cleanup.
check_leftover_by_name() {
  local name
  for name in "${PERF_LEFTOVER_NAMES[@]}"; do
    if pgrep -x "$name" >/dev/null 2>&1; then
      echo "ERROR: Leftover process(es) still running: $name. Please kill them manually (e.g. pkill -KILL -x $name) and retry." >&2
      exit 1
    fi
  done
}

# Check if any tracing_node mainboard still running from a previous run.
check_tracing_node_leftover() {
  if pgrep -f "$TRACING_NODE_PATTERN" >/dev/null 2>&1; then
    echo "ERROR: Leftover tracing_node mainboard is still running. Please kill it manually (e.g. pkill -KILL -f \"$TRACING_NODE_PATTERN\") and retry." >&2
    exit 1
  fi
}

# Check if any of our started processes (tracked in pids) are still running; exit 1 if so.
# Removes dead/zombie PIDs from the array to avoid accumulation and PID-reuse false positives.
check_leftover() {
  local pid i stat to_remove=()
  for i in "${!pids[@]}"; do
    pid="${pids[$i]}"
    [ -z "$pid" ] && continue
    if ! kill -0 "$pid" 2>/dev/null; then
      to_remove+=("$pid")
      continue
    fi
    stat=$(ps -o stat= -p "$pid" 2>/dev/null) || stat=""
    case "$stat" in
      *Z*) to_remove+=("$pid"); continue ;;  # Zombie: will be reaped, remove from tracking
    esac
    echo "ERROR: Leftover process (PID $pid) still running. Please kill it manually (e.g. kill -KILL $pid) and retry." >&2
    exit 1
  done
  for pid in "${to_remove[@]}"; do
    remove_pid "$pid"
  done
}

# Kill leftover processes with INT only (no KILL), then verify cleanup; exit 1 if still leftover
cleanup_leftover_int_only() {
  kill_perf_processes_by_name "-INT"
  kill_tracing_node_by_cmd "-INT"
  sleep_with_timeout 2
  check_leftover_by_name
  check_tracing_node_leftover
}

kill_pid() {
  local pid="$1"
  local is_pgroup="$2"
  local sig="${3:-TERM}"
  if [ -z "$pid" ]; then
    return
  fi
  if [ "$is_pgroup" -eq 1 ]; then
    kill -"$sig" -- "-$pid" 2>/dev/null || true
  else
    kill -"$sig" "$pid" 2>/dev/null || true
  fi
}

# Stop process with TERM then INT; exit 1 with ERROR if still alive
stop_pid_and_check() {
  local pid="$1"
  local is_pgroup="$2"
  local proc_name="$3"
  kill_pid "$pid" "$is_pgroup" "TERM"
  sleep_with_timeout 1
  kill_pid "$pid" "$is_pgroup" "INT"
  sleep_with_timeout 2
  if kill -0 "$pid" 2>/dev/null; then
    echo "ERROR: Process $pid ($proc_name) did not exit after INT. Please kill it manually (e.g. kill -KILL $pid) and retry." >&2
    exit 1
  fi
  remove_pid "$pid"
}

kill_all() {
  if [ "$CLEANED" -eq 1 ]; then
    return
  fi
  CLEANED=1
  echo "Stopping..."
  # Ignore INT/TERM during cleanup so second Ctrl+C cannot interrupt (signals ignored)
  trap '' INT TERM
  exec 3>&2
  exec 2>/dev/null
  # Use plain sleep (not sleep_with_timeout): check_timeout would call kill_all and exit
  # mid-cleanup, leaving processes behind. We must finish cleanup.
  if [ "${#pids[@]}" -gt 0 ]; then
    for i in "${!pids[@]}"; do
      kill_pid "${pids[$i]}" "${pg_kill[$i]}" "TERM"
    done
    sleep 1
    for i in "${!pids[@]}"; do
      kill_pid "${pids[$i]}" "${pg_kill[$i]}" "INT"
    done
    sleep 2
    # Wait for our tracked PIDs only (not unrelated background jobs); reaps zombies
    for i in "${!pids[@]}"; do
      wait "${pids[$i]}" 2>/dev/null || true
    done
  fi
  kill_perf_processes_by_name "-INT"
  sleep 2
  exec 2>&3
  exec 3>&-
  check_leftover
  # Restore pre-script INT/TERM handler (in case of second Ctrl+C before exit completes)
  if [ -n "$__saved_int_term" ]; then
    eval "$__saved_int_term"
  else
    trap - INT TERM
  fi
}

# Function to check if timeout has been reached
check_timeout() {
  local current_time
  current_time=$(date +%s)
  local elapsed=$((current_time - START_TIME))

  if [ "$elapsed" -ge "$TIMEOUT_DURATION" ]; then
    echo "Timeout: Script has been running for more than $TIMEOUT_DURATION seconds"
    kill_all
    exit 1
  fi
}

# Sleep for N seconds, checking timeout between chunks. Do NOT use inside kill_all.
sleep_with_timeout() {
  local remaining="$1"
  while [ "$remaining" -gt 0 ]; do
    check_timeout
    if [ "$remaining" -ge 1 ]; then
      sleep 1
      remaining=$((remaining - 1))
    else
      sleep "$remaining"
      remaining=0
    fi
  done
}

# Capture actual INT/TERM trap state before we modify it (e.g. if sourced or parent set traps)
__saved_int_term=$(trap -p INT TERM 2>/dev/null) || true
trap 'echo "interrupt"; exit 130' INT TERM
trap 'kill_all' EXIT

must_exist() {
  local path="$1"
  if [ ! -x "$path" ]; then
    echo "Error: executable not found: $path"
    exit 1
  fi
}

TOPIC_B="${BIN_DIR}/segar_node_b"
TOPIC_A="${BIN_DIR}/segar_node_a"
SERVICE_SRV="${BIN_DIR}/segar_service"
SERVICE_CLI="${BIN_DIR}/segar_client"
SERVICE_CLI_ASYNC="${BIN_DIR}/segar_client_async"
ACTION_SRV="${BIN_DIR}/segar_action_server"
ACTION_CLI="${BIN_DIR}/segar_action_client_sync"

must_exist "$TOPIC_B"
must_exist "$TOPIC_A"
must_exist "$SERVICE_SRV"
must_exist "$SERVICE_CLI"
must_exist "$SERVICE_CLI_ASYNC"
must_exist "$ACTION_SRV"
must_exist "$ACTION_CLI"

echo "Cleaning pre-existing performance test processes (current user)"
cleanup_leftover_int_only

monitor_pair_until_client_exit() {
  local client_pid="$1"
  local client_name="$2"
  local server_pid="$3"
  local server_name="$4"

  while kill -0 "$client_pid" 2>/dev/null; do
    # Check for timeout during monitoring
    check_timeout
    
    if ! kill -0 "$server_pid" 2>/dev/null; then
      wait "$client_pid" 2>/dev/null || true
      remove_pid "$client_pid"
      echo "Error: ${server_name} exited while ${client_name} running"
      return 1
    fi
    # Sleep 0.2 with timeout check every 0.1s to avoid delayed timeout detection
    check_timeout
    sleep 0.1
    check_timeout
    sleep 0.1
  done

  wait "$client_pid"
  local st=$?
  remove_pid "$client_pid"
  if [ "$st" -ne 0 ]; then
    echo "Error: ${client_name} exited abnormally, status ${st}"
    return 1
  fi
  return 0
}

echo "Running performance test... Log directory: ${LOG_DIR}"

if [ -f "$TRACING_NODE_DAG" ]; then
  if ! command -v mainboard >/dev/null 2>&1; then
    echo "Error: mainboard not found; cannot start tracing_node"
    exit 1
  fi
  echo ""
  echo "== Tracing: start tracing_node =="
  tracing_node_log="${LOG_DIR}/tracing_node.log"
  start_bg mainboard -d "$TRACING_NODE_DAG" "$tracing_node_log"
  tracing_node_pid=$REPLY_PID
  tracing_node_pg=$REPLY_PG
  sleep_with_timeout 1
else
  echo "Warning: tracing_node.dag not found, tracing disabled."
fi

echo ""
echo "== Topic perf: segar_node_b -> segar_node_a =="
topic_b_log="${LOG_DIR}/topic_node_b.log"
topic_a_log="${LOG_DIR}/topic_node_a.log"

start_bg "${TOPIC_B}" "${topic_b_log}"
topic_b_pid=$REPLY_PID
topic_b_pg=$REPLY_PG
sleep_with_timeout 1

start_bg "${TOPIC_A}" "${topic_a_log}"
topic_a_pid=$REPLY_PID
topic_a_pg=$REPLY_PG

if ! monitor_pair_until_client_exit "$topic_a_pid" "segar_node_a" "$topic_b_pid" "segar_node_b"; then
  exit 1
fi

stop_pid_and_check "$topic_b_pid" "$topic_b_pg" "segar_node_b"

echo ""
echo "== Service-client perf: segar_service -> segar_client =="
service_server_log="${LOG_DIR}/service_server.log"
service_client_log="${LOG_DIR}/service_client.log"

SERVICE_NAME="${SERVICE_NAME:-perf_service}"
SERVICE_TIMEOUT_MS="${SERVICE_TIMEOUT_MS:-2000}"

start_bg "${SERVICE_SRV}" "${SERVICE_NAME}" "${service_server_log}"
service_srv_pid=$REPLY_PID
service_srv_pg=$REPLY_PG
sleep_with_timeout 1

start_bg "${SERVICE_CLI}" "--service=${SERVICE_NAME}" "--timeout_ms=${SERVICE_TIMEOUT_MS}" "${service_client_log}"
service_cli_pid=$REPLY_PID
service_cli_pg=$REPLY_PG

if ! monitor_pair_until_client_exit "$service_cli_pid" "segar_client" "$service_srv_pid" "segar_service"; then
  exit 1
fi

stop_pid_and_check "$service_srv_pid" "$service_srv_pg" "segar_service"

echo ""
echo "== Service-client async perf: segar_service -> segar_client_async =="
service_client_async_log="${LOG_DIR}/service_client_async.log"
service_server_async_log="${LOG_DIR}/service_server_async.log"

start_bg "${SERVICE_SRV}" "${SERVICE_NAME}" "${service_server_async_log}"
service_srv_pid=$REPLY_PID
service_srv_pg=$REPLY_PG
sleep_with_timeout 1

start_bg "${SERVICE_CLI_ASYNC}" "--service=${SERVICE_NAME}" "--timeout_ms=${SERVICE_TIMEOUT_MS}" "${service_client_async_log}"
service_cli_async_pid=$REPLY_PID
service_cli_async_pg=$REPLY_PG

# Monitor: detect crash (server or client abnormal exit); when segar_client_async ends, kill segar_service
if ! monitor_pair_until_client_exit "$service_cli_async_pid" "segar_client_async" "$service_srv_pid" "segar_service"; then
  exit 1
fi

stop_pid_and_check "$service_srv_pid" "$service_srv_pg" "segar_service"

echo ""
echo "== Action perf: segar_action_server -> segar_action_client_sync =="
action_server_log="${LOG_DIR}/action_server.log"
action_client_log="${LOG_DIR}/action_client_sync.log"

start_bg "${ACTION_SRV}" "${action_server_log}"
action_srv_pid=$REPLY_PID
action_srv_pg=$REPLY_PG
sleep_with_timeout 1

start_bg "${ACTION_CLI}" "${action_client_log}"
action_cli_pid=$REPLY_PID
action_cli_pg=$REPLY_PG

if ! monitor_pair_until_client_exit "$action_cli_pid" "segar_action_client_sync" "$action_srv_pid" "segar_action_server"; then
  exit 1
fi

stop_pid_and_check "$action_srv_pid" "$action_srv_pg" "segar_action_server"

if [ -n "$tracing_node_pid" ]; then
  stop_pid_and_check "$tracing_node_pid" "$tracing_node_pg" "tracing_node"
fi

echo ""
echo "All processes finished. Checking logs..."
check_timeout

CHECK_SCRIPT="$INTEGRATION_DIR/check_perf_logs.py"
if [ ! -f "$CHECK_SCRIPT" ]; then
  echo "Error: log check script not found: $CHECK_SCRIPT"
  exit 1
fi

# Build CHECK_ARGS from env vars (var_name:option_name)
CHECK_ARGS=()
for spec in \
  TOPIC_MAX_P99_US:topic-max-p99-us \
  TOPIC_MIN_THROUGHPUT_MBPS:topic-min-throughput-mbps \
  TOPIC_MIN_ITERS:topic-min-iters \
  SERVICE_MAX_P99_US:service-max-p99-us \
  SERVICE_MIN_TPS:service-min-tps \
  SERVICE_MIN_SUCCESS_RATE:service-min-success-rate \
  ACTION_MAX_P99_MS:action-max-p99-ms \
  ACTION_MAX_LOSS:action-max-loss \
  ACTION_MIN_THROUGHPUT_MBPS:action-min-throughput-mbps \
  ACTION_MIN_SUCCESS_RATE:action-min-success-rate \
  ACTION_MIN_ITERS:action-min-iters; do
  var="${spec%%:*}"
  opt="${spec#*:}"
  if [ -n "${!var:-}" ]; then
    CHECK_ARGS+=(--"$opt" "${!var}")
  fi
done

# Run log check with remaining timeout; avoids unbounded block during validation
remaining=$((TIMEOUT_DURATION - ($(date +%s) - START_TIME)))
if [ "$remaining" -lt 10 ]; then
  remaining=10
fi
if command -v timeout >/dev/null 2>&1; then
  timeout "$remaining" python3 "$CHECK_SCRIPT" "$LOG_DIR" "${CHECK_ARGS[@]}"
else
  python3 "$CHECK_SCRIPT" "$LOG_DIR" "${CHECK_ARGS[@]}"
fi
