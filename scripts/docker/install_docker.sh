#!/usr/bin/env bash

set -e

echo "=== Docker 国内环境完整配置（Ubuntu 仓库 docker.io，不访问 download.docker.com） ==="

# 若之前尝试过 Docker CE 源但失败，避免 apt 仍指向不可达的源
if [ -f /etc/apt/sources.list.d/docker.list ]; then
    echo "移除旧的 Docker CE 源列表（改用 Ubuntu 自带 docker.io）..."
    sudo rm -f /etc/apt/sources.list.d/docker.list
fi

# 1. 如果已安装 docker，则只做加速与权限配置
if command -v docker &> /dev/null; then
    echo "检测到系统已安装 Docker，跳过安装步骤。"
else
    echo "通过 Ubuntu 官方仓库安装 docker.io（走系统 apt 镜像，无需 Docker 官网）..."
    sudo apt-get update
    sudo apt-get install -y docker.io
fi

# 2. 配置镜像加速器
echo "配置 Docker 镜像加速器..."
sudo mkdir -p /etc/docker
sudo tee /etc/docker/daemon.json << EOF
{
  "registry-mirrors": [
    "https://docker.mirrors.ustc.edu.cn",
    "https://hub-mirror.c.163.com",
    "https://mirror.baidubce.com",
    "https://dockerproxy.com",
    "https://docker.m.daocloud.io",
    "https://docker.nju.edu.cn"
  ],
  "max-concurrent-downloads": 10,
  "max-download-attempts": 3,
  "log-driver": "json-file",
  "log-level": "warn",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  },
  "data-root": "/var/lib/docker"
}
EOF

# 3. 启动并重启 Docker
echo "启动并重启 Docker 服务..."
sudo systemctl daemon-reload || true
sudo systemctl enable docker || true
sudo systemctl restart docker

# 4. 配置 docker 用户组与当前用户权限
echo "配置 docker 用户组与当前用户权限..."
if ! getent group docker >/dev/null; then
    sudo groupadd docker
fi
sudo usermod -aG docker "$USER"

echo
echo "=== 配置完成 ==="
echo "如果想在当前终端立刻无 sudo 使用 docker，请执行："
echo "  newgrp docker"
echo "否则，重新登录后生效。"
