#!/usr/bin/env bash
set -euo pipefail

CONTAINER_NAME="${CONTAINER_NAME:-segar-dev}"
EXEC_USER="${EXEC_USER:-segar}"
CONTAINER_WORKDIR="${CONTAINER_WORKDIR:-/workspace}"

if ! docker ps --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
  echo "容器未运行或不存在：${CONTAINER_NAME}"
  echo "可先运行：./create.sh"
  exit 1
fi

exec docker exec -it -u "${EXEC_USER}" -w "${CONTAINER_WORKDIR}" "${CONTAINER_NAME}" bash

