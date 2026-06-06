#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INTEGRATION_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$INTEGRATION_DIR/bin"
CONFIG_FILE="$SCRIPT_DIR/sensor_config.json"

if [ ! -f "$CONFIG_FILE" ]; then
    echo "错误: 找不到配置文件 $CONFIG_FILE"
    exit 1
fi

if [ ! -x "$BIN_DIR/sensor_node" ]; then
    echo "错误: 找不到可执行文件 $BIN_DIR/sensor_node"
    exit 1
fi

# Optional preload for local profiling/debug; skip if missing.
PRELOAD_LIB="yoyo.so"

# 启动所有15个传感器节点，每个作为独立进程
for i in {1..15}; do
    echo "启动传感器节点 $i..."
    if [ -f "$PRELOAD_LIB" ]; then
        LD_PRELOAD="$PRELOAD_LIB" "$BIN_DIR/sensor_node" "$CONFIG_FILE" "$i" &
    else
        "$BIN_DIR/sensor_node" "$CONFIG_FILE" "$i" &
    fi
done

echo "所有传感器节点已启动"
echo "按 Ctrl+C 停止所有进程"

# 等待所有后台进程
wait
