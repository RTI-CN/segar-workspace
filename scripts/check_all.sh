#!/usr/bin/env bash
# Find PIDs by process name, check whether demo processes are alive, and print a summary
# Usage: ./scripts/check_all.sh

set -e

# Process names corresponding to demos started by start_all.sh
# Linux comm is limited to 15 chars; use `pgrep -f` for longer names
PROCESS_NAMES=(
  topic_talker
  topic_listener
  service_server
  service_client_sync
  service_client_async
  param_server
  param_client
  action_server
  action_client_sync
  action_client_async
  tasker
)
PROCESS_PATTERNS=( "timer.dag|timer_component" "common.dag|common_component" )

running=0
exited=0
echo "---"
for name in "${PROCESS_NAMES[@]}"; do
  if [ ${#name} -gt 15 ]; then
    pids=$(pgrep -f "$name" 2>/dev/null || true)
  else
    pids=$(pgrep -x "$name" 2>/dev/null || true)
  fi
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
