#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="${1:-$PROJECT_ROOT/build_x86/output}"
LOG_DIR="${TMPDIR:-/tmp}/segar_workspace_smoke.$(date +%s).$$"
mkdir -p "$LOG_DIR"
export PYTHONUNBUFFERED=1

declare -a PIDS=()
declare -a PASSED_STEPS=()
LAST_PID=""
declare -A PID_CXX_NAME=()

readonly CXX_PROCESS_NAMES=(
  topic_listener
  topic_talker
  service_server
  service_client_sync
  service_client_async
  action_server
  action_client_sync
  action_client_async
  param_server
  param_client
  wait_event_listener
  wait_event_talker
  zero_copy_listener
  zero_copy_talker
  type_coverage_listener
  type_coverage_talker
)

readonly MAINBOARD_PROCESS_PATTERNS=(
  "component_example/timer_component/.*/config/timer.dag"
)

readonly PYTHON_PROCESS_PATTERNS=(
  "/src_python/topic_example/topic_listener/src/topic_listener.py"
  "/src_python/topic_example/topic_talker/src/topic_talker.py"
  "/src_python/service_example/service_server/src/service_server.py"
  "/src_python/service_example/service_client_sync/src/service_client_sync.py"
  "/src_python/service_example/service_client_async/src/service_client_async.py"
  "/src_python/action_example/action_server/src/action_server.py"
  "/src_python/action_example/action_client_sync/src/action_client_sync.py"
  "/src_python/action_example/action_client_async/src/action_client_async.py"
  "/src_python/param_example/param_server/src/param_server.py"
  "/src_python/param_example/param_client/src/param_client.py"
  "/src_python/usr_msg_topic_example/type_coverage_listener/src/type_coverage_listener.py"
  "/src_python/usr_msg_topic_example/type_coverage_talker/src/type_coverage_talker.py"
)

is_cxx_name() {
  local needle="$1"
  for n in "${CXX_PROCESS_NAMES[@]}"; do
    [[ "$n" == "$needle" ]] && return 0
  done
  return 1
}

cleanup_known_example_processes() {
  for name in "${CXX_PROCESS_NAMES[@]}"; do
    pkill -x "$name" 2>/dev/null || true
  done
  for pattern in "${PYTHON_PROCESS_PATTERNS[@]}"; do
    pkill -f "$pattern" 2>/dev/null || true
  done
  for pattern in "${MAINBOARD_PROCESS_PATTERNS[@]}"; do
    pkill -f "$pattern" 2>/dev/null || true
  done
}

cleanup() {
  for pid in "${PIDS[@]:-}"; do
    if kill -0 "$pid" 2>/dev/null; then
      kill -- -"${pid}" 2>/dev/null || true
    fi
  done
  sleep 0.2
  for pid in "${PIDS[@]:-}"; do
    if kill -0 "$pid" 2>/dev/null; then
      kill -9 -- -"${pid}" 2>/dev/null || true
    fi
  done
  cleanup_known_example_processes
}
trap cleanup EXIT

fail() {
  echo "[smoke] FAIL: $1" >&2
  echo "[smoke] logs=${LOG_DIR}" >&2
  exit 1
}

check_script() {
  local rel_dir="$1"
  local script="${OUTPUT_DIR}/${rel_dir}/scripts/launch.sh"
  if [[ ! -f "$script" ]]; then
    fail "missing launch script: ${script}"
  fi
}

start_bg() {
  local name="$1"
  local rel_dir="$2"
  check_script "$rel_dir"
  local script="${OUTPUT_DIR}/${rel_dir}/scripts/launch.sh"
  local log="${LOG_DIR}/${name}.log"
  echo "[smoke] start ${name}"
  setsid bash "$script" >"$log" 2>&1 &
  LAST_PID="$!"
  PIDS+=("$LAST_PID")
  local base_name
  base_name="$(basename "$rel_dir")"
  if is_cxx_name "$base_name"; then
    PID_CXX_NAME["$LAST_PID"]="$base_name"
  fi
}

process_group_alive() {
  local pgid="$1"
  pgrep -g "$pgid" >/dev/null 2>&1
}

stop_bg() {
  local pid="$1"
  if ! process_group_alive "$pid"; then
    wait "$pid" 2>/dev/null || true
    return
  fi
  kill -- -"${pid}" 2>/dev/null || true
  local deadline=$((SECONDS + 3))
  while process_group_alive "$pid" && ((SECONDS < deadline)); do
    sleep 0.1
  done
  if process_group_alive "$pid"; then
    kill -9 -- -"${pid}" 2>/dev/null || true
    local kill_deadline=$((SECONDS + 3))
    while process_group_alive "$pid" && ((SECONDS < kill_deadline)); do
      sleep 0.1
    done
  fi

  # Some demos may detach from the session/process-group; enforce-stop by process name (C++ demos only).
  local cxx_name="${PID_CXX_NAME[$pid]:-}"
  if [[ -n "$cxx_name" ]]; then
    pkill -x "$cxx_name" 2>/dev/null || true
    sleep 0.1
    pkill -9 -x "$cxx_name" 2>/dev/null || true
  fi
  wait "$pid" 2>/dev/null || true
}

wait_log() {
  local log="$1"
  local pattern="$2"
  local timeout_sec="$3"
  local deadline=$((SECONDS + timeout_sec))
  while ((SECONDS < deadline)); do
    if [[ -f "$log" ]] && rg -q --fixed-strings "$pattern" "$log"; then
      return 0
    fi
    sleep 0.2
  done
  echo "[smoke] timeout waiting pattern: ${pattern}" >&2
  if [[ -f "$log" ]]; then
    tail -n 120 "$log" >&2 || true
  fi
  return 1
}

run_step() {
  local step_name="$1"
  shift
  echo "[smoke] ${step_name}"
  "$@"
  PASSED_STEPS+=("$step_name")
}

run_cpp_topic_smoke() {
  start_bg "cpp_topic_listener" "topic_example/topic_listener"
  local listener_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_topic_listener.log" "Waiting for messages..." 8
  start_bg "cpp_topic_talker" "topic_example/topic_talker"
  local talker_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_topic_listener.log" "Received message:" 10
  stop_bg "$talker_pid"
  stop_bg "$listener_pid"
}

run_cpp_service_sync_smoke() {
  start_bg "cpp_service_server_sync" "service_example/service_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_service_server_sync.log" "Waiting for requests..." 8
  start_bg "cpp_service_client_sync" "service_example/service_client_sync"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_service_client_sync.log" "Response msg:Camera info set successfully" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_cpp_service_async_smoke() {
  start_bg "cpp_service_server_async" "service_example/service_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_service_server_async.log" "Waiting for requests..." 8
  start_bg "cpp_service_client_async" "service_example/service_client_async"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_service_client_async.log" "Response msg:Camera info set successfully" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_cpp_action_sync_smoke() {
  start_bg "cpp_action_server_sync" "action_example/action_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_action_server_sync.log" "Action server started successfully" 8
  start_bg "cpp_action_client_sync" "action_example/action_client_sync"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_action_client_sync.log" "[Sync] Result received:" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_cpp_action_async_smoke() {
  start_bg "cpp_action_server_async" "action_example/action_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_action_server_async.log" "Action server started successfully" 8
  start_bg "cpp_action_client_async" "action_example/action_client_async"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_action_client_async.log" "[Async] Result received:" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_cpp_param_smoke() {
  start_bg "cpp_param_server" "param_example/param_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_param_server.log" "Parameter server started successfully" 8
  start_bg "cpp_param_client" "param_example/param_client"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_param_client.log" "p3_pb:" 10
  wait_log "${LOG_DIR}/cpp_param_client.log" "Remote parameters count:" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_cpp_concurrent_wait_event_smoke() {
  start_bg "cpp_wait_event_listener" "concurrent_example/wait_event_listener"
  local listener_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_wait_event_listener.log" "Waiting for wait_event requests..." 8
  start_bg "cpp_wait_event_talker" "concurrent_example/wait_event_talker"
  local talker_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_wait_event_listener.log" "Received go and can continue work" 10
  stop_bg "$talker_pid"
  stop_bg "$listener_pid"
}

run_cpp_zero_copy_smoke() {
  start_bg "cpp_zero_copy_listener" "zero_copy_example/zero_copy_listener"
  local listener_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_zero_copy_listener.log" "Waiting for zero-copy images..." 8
  start_bg "cpp_zero_copy_talker" "zero_copy_example/zero_copy_talker"
  local talker_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_zero_copy_listener.log" "Received zero-copy image:" 10
  stop_bg "$talker_pid"
  stop_bg "$listener_pid"
}

run_cpp_component_timer_smoke() {
  start_bg "cpp_component_timer" "component_example/timer_component"
  local timer_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_component_timer.log" "timer_component_example: Write image_front/image_rear" 10
  stop_bg "$timer_pid"
}

run_cpp_usr_msg_topic_smoke() {
  start_bg "cpp_usr_msg_type_coverage_listener" "usr_msg_topic_example/type_coverage_listener"
  local listener_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_usr_msg_type_coverage_listener.log" "Waiting for TypeCoverage messages..." 8
  start_bg "py_usr_msg_type_coverage_talker_to_cpp" "src_python/usr_msg_topic_example/type_coverage_talker"
  local talker_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_usr_msg_type_coverage_listener.log" "Received TypeCoverage int32=" 10
  wait_log "${LOG_DIR}/cpp_usr_msg_type_coverage_listener.log" "py_memoryview=[208, 209, 210]" 10
  stop_bg "$talker_pid"
  stop_bg "$listener_pid"
}

run_python_topic_smoke() {
  start_bg "py_topic_listener" "src_python/topic_example/topic_listener"
  local listener_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_topic_listener.log" "[topic_listener] waiting for messages..." 8
  start_bg "py_topic_talker" "src_python/topic_example/topic_talker"
  local talker_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_topic_listener.log" "[topic_listener] recv:" 10
  stop_bg "$talker_pid"
  stop_bg "$listener_pid"
}

run_python_usr_msg_topic_smoke() {
  start_bg "py_usr_msg_type_coverage_listener" "src_python/usr_msg_topic_example/type_coverage_listener"
  local listener_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_usr_msg_type_coverage_listener.log" "[py_usr_msg_type_coverage_listener] waiting for TypeCoverage messages..." 8
  start_bg "cpp_usr_msg_type_coverage_talker_to_py" "usr_msg_topic_example/type_coverage_talker"
  local talker_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_usr_msg_type_coverage_listener.log" "[py_usr_msg_type_coverage_listener] recv: int32=" 10
  wait_log "${LOG_DIR}/py_usr_msg_type_coverage_listener.log" "py_memoryview=[208, 209, 210]" 10
  stop_bg "$talker_pid"
  stop_bg "$listener_pid"
}

run_python_service_sync_smoke() {
  start_bg "py_service_server_sync" "src_python/service_example/service_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_service_server_sync.log" "[service_server] waiting for requests..." 8
  start_bg "py_service_client_sync" "src_python/service_example/service_client_sync"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_service_client_sync.log" "response=Camera info set successfully" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_python_service_async_smoke() {
  start_bg "py_service_server_async" "src_python/service_example/service_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_service_server_async.log" "[service_server] waiting for requests..." 8
  start_bg "py_service_client_async" "src_python/service_example/service_client_async"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_service_client_async.log" "response=Camera info set successfully" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_python_action_sync_smoke() {
  start_bg "py_action_server_sync" "src_python/action_example/action_server"
  local server_pid="$LAST_PID"
  sleep 1
  start_bg "py_action_client_sync" "src_python/action_example/action_client_sync"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_action_client_sync.log" "[action_client_sync] result goal=" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_python_action_async_smoke() {
  start_bg "py_action_server_async" "src_python/action_example/action_server"
  local server_pid="$LAST_PID"
  sleep 1
  start_bg "py_action_client_async" "src_python/action_example/action_client_async"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_action_client_async.log" "[action_client_async] result goal=" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_python_action_client_cpp_server_smoke() {
  start_bg "cpp_action_server_for_py_client" "action_example/action_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_action_server_for_py_client.log" "Action server started successfully" 8
  start_bg "py_action_client_to_cpp_server" "src_python/action_example/action_client_sync"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_action_client_to_cpp_server.log" "[action_client_sync] result goal=" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_cpp_action_client_python_server_smoke() {
  start_bg "py_action_server_for_cpp_client" "src_python/action_example/action_server"
  local server_pid="$LAST_PID"
  sleep 1
  start_bg "cpp_action_client_to_py_server" "action_example/action_client_sync"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/cpp_action_client_to_py_server.log" "[Sync] Result received:" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

run_python_param_smoke() {
  start_bg "py_param_server" "src_python/param_example/param_server"
  local server_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_param_server.log" "[param_server] local params=" 8
  start_bg "py_param_client" "src_python/param_example/param_client"
  local client_pid="$LAST_PID"
  wait_log "${LOG_DIR}/py_param_client.log" "[param_client] p1_int=1 p2_string=test" 10
  wait_log "${LOG_DIR}/py_param_client.log" "[param_client] p3_pb module_name=" 10
  wait_log "${LOG_DIR}/py_param_client.log" "[param_client] remote params=" 10
  stop_bg "$client_pid"
  stop_bg "$server_pid"
}

main() {
  if [[ ! -d "$OUTPUT_DIR" ]]; then
    fail "output dir not found: ${OUTPUT_DIR}"
  fi

  cleanup_known_example_processes

  run_step "c++ topic" run_cpp_topic_smoke
  run_step "c++ service sync" run_cpp_service_sync_smoke
  run_step "c++ service async" run_cpp_service_async_smoke
  run_step "c++ action sync" run_cpp_action_sync_smoke
  run_step "c++ action async" run_cpp_action_async_smoke
  run_step "c++ param" run_cpp_param_smoke
  run_step "c++ concurrent wait_event" run_cpp_concurrent_wait_event_smoke
  run_step "c++ zero_copy" run_cpp_zero_copy_smoke
  run_step "c++ component timer" run_cpp_component_timer_smoke
  run_step "c++ usr_msg topic" run_cpp_usr_msg_topic_smoke
  run_step "python topic" run_python_topic_smoke
  run_step "python usr_msg topic" run_python_usr_msg_topic_smoke
  run_step "python service sync" run_python_service_sync_smoke
  run_step "python service async" run_python_service_async_smoke
  run_step "python action sync" run_python_action_sync_smoke
  run_step "python action async" run_python_action_async_smoke
  run_step "python action client -> c++ server" run_python_action_client_cpp_server_smoke
  run_step "c++ action client -> python server" run_cpp_action_client_python_server_smoke
  run_step "python param" run_python_param_smoke

  echo "[smoke] PASS logs=${LOG_DIR}"
  echo "[smoke] Summary:"
  for step in "${PASSED_STEPS[@]}"; do
    echo "[smoke]   PASS ${step}"
  done
}

main "$@"
