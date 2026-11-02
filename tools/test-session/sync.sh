#!/bin/sh
# SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
# SPDX-License-Identifier: CC0-1.0
#
# Copies the source tree to the test container on porygon (LXC 128 on arceus), then builds and installs it there.
#   sync.sh [HOST] [CONTAINER]
set -eu

host="${1:-porygon}"
container="${2:-pk-desktop}"
root="$(cd "$(dirname "$0")/../.." && pwd)"

rsync -a --delete --exclude .git --exclude '/build*' "$root/" "$host:pk/src/"
ssh "$host" podman exec -i -u root "$container" sh -s <<'SCRIPT'
set -e
cd /src
# Fedora ships an older libplasma than master expects; the API used is the same
sed -i 's/set(PROJECT_DEP_VERSION "6.7.90")/set(PROJECT_DEP_VERSION "6.7.0")/' CMakeLists.txt
cmake -S . -B /build -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON >/dev/null 2>&1
ninja -C /build install | tail -1
cd tools/test-session
wayland-scanner client-header /usr/share/plasma-wayland-protocols/fake-input.xml fake-input-client-protocol.h
wayland-scanner private-code /usr/share/plasma-wayland-protocols/fake-input.xml fake-input-protocol.c
cc -o /usr/local/bin/touch-inject touch.c fake-input-protocol.c $(pkg-config --cflags --libs wayland-client)
c++ -std=c++20 -fPIC -o /usr/local/bin/pk-screenshot screenshot.cpp $(pkg-config --cflags --libs Qt6DBus Qt6Gui)
install -m755 session.sh /usr/local/bin/pk-session
SCRIPT
