# CI builder (DockerHub public image)
#   docker build --target builder -t rbrich/xcikit-debian-12 docker/debian-12/
#   docker run --rm -v $PWD:/src -w /src -it rbrich/xcikit-debian-12 bash
#
# SSH builder (for Clion "Remote Host" toolchain)
# see https://blog.jetbrains.com/clion/2020/01/using-docker-with-clion/)
#   docker build --target builder-ssh --build-arg rootpw=pass1 -t xcikit-debian-ssh docker/debian/
#   docker run -p 127.0.0.1:2222:22 --cap-add sys_ptrace --name xcikit-debian-ssh xcikit-debian-ssh
#   ssh root@localhost -p 2222
# Cleanup known hosts:
#   ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"
# Example cmake arguments:
#   -DRUN_CONAN=1 -DFORCE_COLORS=1

FROM debian:bookworm-slim AS builder

RUN echo "gcc 11"; apt-get update && apt-get install --no-install-recommends -y \
    g++-11 && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 50 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 50 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++    50 && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/gcc      50

RUN echo "dev tools"; apt-get update && apt-get install --no-install-recommends -y \
    git curl ca-certificates \
    cmake make ninja-build pkg-config \
    python3-minimal libpython3-stdlib && \
    rm -rf /var/lib/apt/lists/*
ENV CMAKE_GENERATOR=Ninja CONAN_CMAKE_GENERATOR=Ninja

RUN echo "conan"; apt-get update && apt-mark manual $(apt-mark showauto) && \
    apt-get install --no-install-recommends -y \
    python3-pip && pip3 --no-cache-dir install conan && \
    conan profile new default --detect && \
    conan profile update "settings.compiler.libcxx=libstdc++11" default && \
    conan config set general.revisions_enabled=1 && \
    apt-get purge -y python3-pip $(apt-mark showauto) && \
    rm -rf /var/lib/apt/lists/*

RUN echo "xcikit deps"; apt-get update && apt-get install --no-install-recommends -y \
    librange-v3-dev libglfw3-dev glslang-tools libvulkan-dev libfreetype6-dev \
    libhyperscan-dev catch2 libbenchmark-dev && \
    rm -rf /var/lib/apt/lists/*

RUN echo "all-from-conan deps"; apt-get update && apt-get install --no-install-recommends -y \
    xorg-dev libx11-xcb-dev libxcb-render0-dev libxcb-render-util0-dev libxcb-xkb-dev \
    libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev \
    libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev libxcb-xinerama0-dev \
    xkb-data libxcb-dri3-dev libxcb-util0-dev && \
    rm -rf /var/lib/apt/lists/*


# --------------------------------------------------------------------------------------------------
# https://docs.docker.com/engine/examples/running_ssh_service/
FROM builder AS builder-ssh

ARG rootpw

RUN apt-get update && apt-get install --no-install-recommends -y gdb rsync openssh-server
RUN echo "root:$rootpw" | chpasswd && mkdir /run/sshd && \
    echo 'PermitRootLogin yes' > "/etc/ssh/sshd_config.d/docker.conf"

EXPOSE 22

# -D    do not become a daemon
CMD ["/usr/sbin/sshd", "-D"]
