#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ_DIR="$(cd "$SCRIPT_DIR/../../../../" && pwd)"

if [ -f "$PROJ_DIR/segar_setup.bash" ]; then
  # shellcheck disable=SC1091
  source "$PROJ_DIR/segar_setup.bash"
fi

export LD_LIBRARY_PATH="$PROJ_DIR/third_party/lib:$PROJ_DIR/lib:${LD_LIBRARY_PATH:-}"
export SEGAR_PATH="$SCRIPT_DIR/.."

PY_VER="$(python3 -c 'import sys; print(f"{sys.version_info[0]}.{sys.version_info[1]}")')"
SITE1="$PROJ_DIR/lib/python$PY_VER/site-packages"
SITE2="$PROJ_DIR/third_party/lib/python$PY_VER/site-packages"
if [ -d "$SITE1" ]; then
  export PYTHONPATH="$SITE1:${PYTHONPATH:-}"
fi
if [ -d "$SITE2" ]; then
  export PYTHONPATH="$SITE2:${PYTHONPATH:-}"
fi

echo "[launch] PYTHONPATH=${PYTHONPATH:-<empty>}"
exec python3 "$SCRIPT_DIR/../src/service_server.py"
