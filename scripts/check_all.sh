#!/usr/bin/env bash
# Find PIDs by process name, check whether demo processes are alive, and print a summary
# Usage: ./scripts/check_all.sh

set -e

# Process names corresponding to demos started by start_all.sh
# Notes:
# - Linux comm is limited to 15 chars, so `pgrep -x` is not reliable for long names (can collide).
# - We use `pgrep -f -a` and filter to the real binary argv0, excluding launch scripts.
PROCESS_NAMES=(
  topic_talker
  topic_listener
  transform_broadcaster
  transform_static_broadcaster
  transform_listener
  transform_static_listener
  service_server
  service_client_sync
  service_client_async
  param_server
  param_client
  action_server
  action_client_sync
  action_client_async
  wait_event_talker
  wait_event_listener
)
PROCESS_PATTERNS=( "timer.dag|timer_component" "common.dag|common_component" "sync.dag|sync_component" )

find_pids_for_name() {
  local name="$1"
  # Output: one PID per line (may be empty)
  pgrep -f -a "$name" 2>/dev/null | awk -v n="$name" '
    {
      pid=$1
      cmd=substr($0, index($0,$2))
      if (cmd ~ /launch\.sh/) next
      if (cmd ~ ("(^|.*/)" n "([[:space:]]|$)")) print pid
    }
  ' || true
}

running=0
exited=0
echo "---"
for name in "${PROCESS_NAMES[@]}"; do
  pids=$(find_pids_for_name "$name")
  if [ -n "$pids" ]; then
    echo "$name $pids"
    running=$((running + 1))
  else
    echo "$name Exited"
    exited=$((exited + 1))
  fi
done
for entry in "${PROCESS_PATTERNS[@]}"; do
  pattern="${entry%%|*}"
  display="${entry##*|}"
  pids=$(pgrep -f "$pattern" 2>/dev/null || true)
  if [ -n "$pids" ]; then
    echo "$display $pids"
    running=$((running + 1))
  else
    echo "$display Exited"
    exited=$((exited + 1))
  fi
done
echo "---"
total=$((${#PROCESS_NAMES[@]} + ${#PROCESS_PATTERNS[@]}))
if [ "$total" -eq 0 ]; then
  echo "All Exited"
elif [ "$running" -eq "$total" ]; then
  echo "All Started"
elif [ "$exited" -eq "$total" ]; then
  echo "All Exited"
else
  echo "Partially Started"
fi
