#!/bin/sh
# SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
# SPDX-License-Identifier: CC0-1.0
#
# Starts a headless KWin session with the installed plasma-keyboard and a text field,
# for testing under a real compositor. Run inside the test container.
#
#   session.sh start [WIDTHxHEIGHT]   start the session (default 1280x800, the Steam Deck)
#   session.sh run CMD...             run a command inside the session
#   session.sh shot FILE.png          screenshot the session
#   session.sh tap X Y                tap the screen
#   session.sh swipe X1 Y1 X2 Y2...   swipe across the screen
#   session.sh stop
set -eu

STATE=/tmp/pk-session
mkdir -p "$STATE"

case "${1:-}" in
start)
    size="${2:-1280x800}"
    "$0" stop >/dev/null 2>&1 || true
    export XDG_RUNTIME_DIR="$STATE/runtime"
    mkdir -p -m 700 "$XDG_RUNTIME_DIR"
    dbus-daemon --session --fork --print-address=1 --address="unix:path=$STATE/bus" >/dev/null
    export DBUS_SESSION_BUS_ADDRESS="unix:path=$STATE/bus"
    # VirtualKeyboardMode=2 shows the panel for pointer input too, not only touch
    export XDG_CONFIG_HOME="$STATE/config"
    mkdir -p "$XDG_CONFIG_HOME"
    kwriteconfig6 --file kwinrc --group Wayland --key VirtualKeyboardMode 2
    export KWIN_WAYLAND_NO_PERMISSION_CHECKS=1 KWIN_SCREENSHOT_NO_PERMISSION_CHECKS=1 QT_QUICK_BACKEND=software KWIN_COMPOSE=O2 LIBGL_ALWAYS_SOFTWARE=1
    kwin_wayland --virtual --width "${size%x*}" --height "${size#*x}" --socket pk-wayland \
        --inputmethod plasma-keyboard \
        >"$STATE/kwin.log" 2>&1 &
    echo $! >"$STATE/kwin.pid"
    sleep 3
    WAYLAND_DISPLAY=pk-wayland QT_QPA_PLATFORM=wayland kwrite >"$STATE/app.log" 2>&1 &
    echo $! >"$STATE/app.pid"
    sleep 4
    echo "session up"
    ;;
run)
    shift
    XDG_RUNTIME_DIR="$STATE/runtime" XDG_CONFIG_HOME="$STATE/config" DBUS_SESSION_BUS_ADDRESS="unix:path=$STATE/bus" WAYLAND_DISPLAY=pk-wayland QT_QPA_PLATFORM=wayland "$@"
    ;;
shot)
    "$0" run pk-screenshot "$2"
    ;;
tap | swipe)
    "$0" run touch-inject "$@"
    ;;
stop)
    for f in app kwin; do
        [ -f "$STATE/$f.pid" ] && kill "$(cat "$STATE/$f.pid")" 2>/dev/null || true
    done
    pkill -f "address=unix:path=$STATE/bus" 2>/dev/null || true
    rm -rf "$STATE"
    ;;
*)
    sed -n '4,14p' "$0"
    exit 1
    ;;
esac
