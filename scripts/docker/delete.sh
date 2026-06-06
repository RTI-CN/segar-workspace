#!/usr/bin/env bash
set -euo pipefail

CONTAINER_NAME="${CONTAINER_NAME:-segar-dev}"

REMOVE_VOLUMES="${REMOVE_VOLUMES:-0}"  # 设为 1 时删除匿名卷（-v）

if docker ps -a --format '{{.Names}}' | grep -qx "${CONTAINER_NAME}"; then
  echo "删除容器: ${CONTAINER_NAME}"
  if [ "${REMOVE_VOLUMES}" = "1" ]; then
    docker rm -f -v "${CONTAINER_NAME}"
  else
    docker rm -f "${CONTAINER_NAME}"
  fi
else
  echo "容器不存在，跳过：${CONTAINER_NAME}"
fi

echo "完成。"

