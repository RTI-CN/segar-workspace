#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ_DIR="$(cd "$SCRIPT_DIR/../../../../" && pwd)"
cd "$SCRIPT_DIR/.."

if [ ! -f "$PROJ_DIR/segar_setup.bash" ]; then
  echo "missing segar_setup.bash: $PROJ_DIR/segar_setup.bash" >&2
  exit 1
fi
# shellcheck disable=SC1091
source "$PROJ_DIR/segar_setup.bash"

export SEGAR_PATH="$SCRIPT_DIR/.."

echo "[launch] PYTHONPATH=${PYTHONPATH:-<empty>}"
exec python3 -u "$SCRIPT_DIR/../src/service_server.py"
