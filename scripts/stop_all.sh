#!/usr/bin/env bash
# Find and stop demo processes started by start_all.sh via process name lookup
# Usage: ./scripts/stop_all.sh

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
PROCESS_PATTERNS=( "timer.dag" "common.dag" "sync.dag" )

find_pids_for_name() {
  local name="$1"
  pgrep -f -a "$name" 2>/dev/null | awk -v n="$name" '
    {
      pid=$1
      cmd=substr($0, index($0,$2))
      if (cmd ~ /launch\.sh/) next
      if (cmd ~ ("(^|.*/)" n "([[:space:]]|$)")) print pid
    }
  ' || true
}

echo "Stopping processes by name/pattern..."
all_pids=()
for name in "${PROCESS_NAMES[@]}"; do
  while read -r pid; do
    [ -n "$pid" ] && all_pids+=( "$pid" )
  done < <(find_pids_for_name "$name")
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
