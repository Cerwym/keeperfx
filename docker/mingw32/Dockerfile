# KeeperFX build image — Windows x86 cross-compile (i686-w64-mingw32)
#
# Bakes in:
#   - MinGW-w64 i686 cross-toolchain (posix threads)
#   - CMake + Ninja
#   - SDL2 / SDL2_image / SDL2_mixer / SDL2_net official MinGW prebuilts
#     installed into /usr/i686-w64-mingw32/ (the CMAKE_FIND_ROOT_PATH for
#     mingw32.cmake) so find_package(SDL2 CONFIG REQUIRED) resolves without vcpkg
#   - kfx-deps tarballs in /opt/kfx-deps (read by KFX_DEPS_CACHE in
#     deps/CMakeLists.txt — avoids downloading at every CI run)
#
# Usage (local):
#   docker run --rm -v $(pwd):/src ghcr.io/<org>/keeperfx-build-mingw32:latest \
#     bash -c "cmake --preset windows-x86-release && cmake --build --preset windows-x86-release"

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# ── Toolchain & build tools ──────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc-mingw-w64-i686-posix \
        g++-mingw-w64-i686-posix \
        cmake \
        ninja-build \
        curl \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ── SDL2 MinGW i686 prebuilts → /usr/i686-w64-mingw32 ───────────────────────
# Versions must match what prebuilds.mk downloads (kept in sync with the repo).
ARG SDL2_VER=2.30.7
ARG SDL2_IMAGE_VER=2.8.2
ARG SDL2_MIXER_VER=2.8.0
ARG SDL2_NET_VER=2.2.0
ARG ARCH=i686-w64-mingw32
ARG PREFIX=/usr/${ARCH}

RUN set -eux; \
    TMP=$(mktemp -d); \
    # SDL2
    curl -fsSL "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VER}/SDL2-devel-${SDL2_VER}-mingw.tar.gz" \
        | tar -xz -C "${TMP}"; \
    cp -r "${TMP}/SDL2-${SDL2_VER}/${ARCH}/include/." "${PREFIX}/include/"; \
    cp -r "${TMP}/SDL2-${SDL2_VER}/${ARCH}/lib/."     "${PREFIX}/lib/"; \
    rm -rf "${TMP}"; \
    # SDL2_image
    TMP=$(mktemp -d); \
    curl -fsSL "https://github.com/libsdl-org/SDL_image/releases/download/release-${SDL2_IMAGE_VER}/SDL2_image-devel-${SDL2_IMAGE_VER}-mingw.tar.gz" \
        | tar -xz -C "${TMP}"; \
    cp -r "${TMP}/SDL2_image-${SDL2_IMAGE_VER}/${ARCH}/include/." "${PREFIX}/include/"; \
    cp -r "${TMP}/SDL2_image-${SDL2_IMAGE_VER}/${ARCH}/lib/."     "${PREFIX}/lib/"; \
    rm -rf "${TMP}"; \
    # SDL2_mixer
    TMP=$(mktemp -d); \
    curl -fsSL "https://github.com/libsdl-org/SDL_mixer/releases/download/release-${SDL2_MIXER_VER}/SDL2_mixer-devel-${SDL2_MIXER_VER}-mingw.tar.gz" \
        | tar -xz -C "${TMP}"; \
    cp -r "${TMP}/SDL2_mixer-${SDL2_MIXER_VER}/${ARCH}/include/." "${PREFIX}/include/"; \
    cp -r "${TMP}/SDL2_mixer-${SDL2_MIXER_VER}/${ARCH}/lib/."     "${PREFIX}/lib/"; \
    rm -rf "${TMP}"; \
    # SDL2_net
    TMP=$(mktemp -d); \
    curl -fsSL "https://github.com/libsdl-org/SDL_net/releases/download/release-${SDL2_NET_VER}/SDL2_net-devel-${SDL2_NET_VER}-mingw.tar.gz" \
        | tar -xz -C "${TMP}"; \
    cp -r "${TMP}/SDL2_net-${SDL2_NET_VER}/${ARCH}/include/." "${PREFIX}/include/"; \
    cp -r "${TMP}/SDL2_net-${SDL2_NET_VER}/${ARCH}/lib/."     "${PREFIX}/lib/"; \
    rm -rf "${TMP}"

# ── kfx-deps tarballs cache → /opt/kfx-deps ─────────────────────────────────
# deps/CMakeLists.txt reads KFX_DEPS_CACHE before downloading from GitHub.
RUN mkdir -p /opt/kfx-deps && cd /opt/kfx-deps && \
    curl -fsSLo enet-mingw32.tar.gz       "https://github.com/dkfans/kfx-deps/releases/download/initial/enet-mingw32.tar.gz" && \
    curl -fsSLo zlib-mingw32.tar.gz       "https://github.com/dkfans/kfx-deps/releases/download/initial/zlib-mingw32.tar.gz" && \
    curl -fsSLo spng-mingw32.tar.gz       "https://github.com/dkfans/kfx-deps/releases/download/initial/spng-mingw32.tar.gz" && \
    curl -fsSLo astronomy-mingw32.tar.gz  "https://github.com/dkfans/kfx-deps/releases/download/initial/astronomy-mingw32.tar.gz" && \
    curl -fsSLo centijson-mingw32.tar.gz  "https://github.com/dkfans/kfx-deps/releases/download/initial/centijson-mingw32.tar.gz" && \
    curl -fsSLo ffmpeg-mingw32.tar.gz     "https://github.com/dkfans/kfx-deps/releases/download/initial/ffmpeg-mingw32.tar.gz" && \
    curl -fsSLo openal-mingw32.tar.gz     "https://github.com/dkfans/kfx-deps/releases/download/2024-11-14/openal-mingw32.tar.gz" && \
    curl -fsSLo luajit-mingw32.tar.gz     "https://github.com/dkfans/kfx-deps/releases/download/20250418/luajit-mingw32.tar.gz" && \
    curl -fsSLo miniupnpc-mingw32.tar.gz  "https://github.com/dkfans/kfx-deps/releases/download/20260102/miniupnpc-mingw32.tar.gz" && \
    curl -fsSLo libnatpmp-mingw32.tar.gz  "https://github.com/dkfans/kfx-deps/releases/download/20260102/libnatpmp-mingw32.tar.gz"

ENV KFX_DEPS_CACHE=/opt/kfx-deps

WORKDIR /src
