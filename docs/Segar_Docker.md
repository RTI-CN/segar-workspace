# Segar Docker

当宿主机环境不方便安装依赖、或需要统一编译/运行环境时，可使用仓库内置的 Docker 脚本快速创建开发容器。

## 1. 安装 Docker（宿主机）

该步骤为**可选**：如果宿主机已安装并可正常使用 Docker，可跳过。

在宿主机执行：

```bash
./scripts/docker/install_docker.sh
```

> 若你的网络无法访问 `download.docker.com`，该脚本会使用 Ubuntu 仓库的 `docker.io` 安装方式，并配置常用镜像加速器。

## 2. 创建/启动容器

在仓库根目录执行：

```bash
./scripts/docker/create.sh
```

默认行为：

- `Dockerfile` 位于：`scripts/docker/Dockerfile`（`create.sh` 会以该目录作为 build context）
- 会构建镜像并创建容器（仅当容器不存在时）
- 会将**当前目录**挂载到容器内的 `/workspace`
- 会将宿主机端口 `2222` 映射到容器 `22`（用于 SSH 登录）
- 会对齐容器内用户 `segar` 的 UID/GID 与宿主机当前用户一致，避免挂载目录权限问题
- 会开启 `--ipc=host`，与宿主机共享 `/dev/shm`

## 3. 进入容器

```bash
./scripts/docker/attach.sh
```

默认行为：以 `segar` 用户进入容器，并将工作目录设为 `/workspace`。

## 4. 删除容器

```bash
./scripts/docker/delete.sh
```

默认行为：仅删除容器本身（不删除镜像）。

## 5. 通过 SSH 登录（可选）

容器内默认已配置 `segar` 用户，用户名/密码均为 `segar`：

```bash
ssh segar@localhost -p 2222
```
