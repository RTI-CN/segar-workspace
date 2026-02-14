#!/usr/bin/env bash
# Find and stop demo processes started by start_all.sh via process name lookup
# Usage: ./scripts/stop_all.sh

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
PROCESS_PATTERNS=( "timer.dag" "common.dag" )

echo "Stopping processes by name/pattern..."
all_pids=()
for name in "${PROCESS_NAMES[@]}"; do
  if [ ${#name} -gt 15 ]; then
    pids_cmd="pgrep -f $name"
  else
    pids_cmd="pgrep -x $name"
  fi
  while read -r pid; do
    [ -n "$pid" ] && all_pids+=( "$pid" )
  done < <(eval "$pids_cmd" 2>/dev/null || true)
done
for pattern in "${PROCESS_PATTERNS[@]}"; do
  while read -r pid; do
    [ -n "$pid" ] && all_pids+=( "$pid" )
  done < <(pgrep -f "$pattern" 2>/dev/null || true)
done

if [ ${#all_pids[@]} -eq 0 ]; then
  echo "No matching processes found."
  exit 0
fi

kill -15 "${all_pids[@]}" 2>/dev/null || true

max_rounds=10
interval=2
round=1
while [ $round -le $max_rounds ]; do
  still_alive=()
  for pid in "${all_pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      still_alive+=( "$pid" )
    fi
  done
  [ ${#still_alive[@]} -eq 0 ] && break
  echo "Waiting for ${#still_alive[@]} process(es)... (${round}/${max_rounds})"
  sleep $interval
  round=$((round + 1))
done

for pid in "${all_pids[@]}"; do
  if kill -0 "$pid" 2>/dev/null; then
    kill -9 "$pid" 2>/dev/null || true
    echo "Killed -9: PID $pid"
  fi
done
echo "Done."
