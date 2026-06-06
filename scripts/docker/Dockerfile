FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

ARG USERNAME=segar
ARG UID=1000
ARG GID=1000

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    sudo \
    net-tools \
    vim \
    git \
    cmake \
    g++-9 \
    gcc-9 \
    curl \
    openjdk-11-jre-headless \
    openssh-server \
    libcurl4-openssl-dev \
    uuid-dev \
    python3-dev \
    python3-pip \
    python3-protobuf \
    libsqlite3-dev \
    sqlite3 \
    python3-pybind11 \
    gcc-11-aarch64-linux-gnu \
    g++-11-aarch64-linux-gnu \
    && rm -rf /var/lib/apt/lists/*

# SSH runtime requirements
RUN mkdir -p /var/run/sshd && \
    ssh-keygen -A

# User: segar / segar (UID/GID 可通过 build args 对齐宿主机)
RUN groupadd -g "${GID}" "${USERNAME}" && \
    useradd -m -s /bin/bash -u "${UID}" -g "${GID}" "${USERNAME}" && \
    echo "${USERNAME}:${USERNAME}" | chpasswd && \
    usermod -aG sudo "${USERNAME}"

# SSH: allow password login; disallow root login
RUN sed -ri 's/^#?PasswordAuthentication\s+.*/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin no/' /etc/ssh/sshd_config && \
    sed -ri 's/^#?UsePAM\s+.*/UsePAM yes/' /etc/ssh/sshd_config

EXPOSE 22

CMD ["/usr/sbin/sshd", "-D"]

