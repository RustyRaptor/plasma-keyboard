#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#
# Makes KWin use this fork's Flatpak (branch "deck") as the only virtual keyboard,
# and hides every other virtual keyboard from System Settings > Virtual Keyboard.
# Only files in your home directory change, so it works on SteamOS's read-only root.
#
#   use-fork-keyboard.sh            switch to the fork
#   use-fork-keyboard.sh --status   say which keyboard KWin starts, which one runs, and which build it is
#   use-fork-keyboard.sh --undo     go back to what was set before

set -euo pipefail

app=org.kde.plasma.keyboard
branch=deck
ours="${XDG_DATA_HOME:-$HOME/.local/share}/flatpak/exports/share/applications/$app.desktop"
overrides="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
state="${XDG_STATE_HOME:-$HOME/.local/state}/plasma-keyboard-fork"

say() { printf '%s\n' "$*"; }
fail() { printf 'error: %s\n' "$*" >&2; exit 1; }

# KWin starts an input method again only when the setting changes, so clear it first.
# KWin doesn't keep other processes from acting as the keyboard, so stop every other one in between.
restart_input_method() {
    kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod ""
    sleep 1
    flatpak kill "$app" 2>/dev/null || true
    pkill -x plasma-keyboard || true
    pkill -x maliit-keyboard || true
    sleep 1
    kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod "$1"
}

# Whether a process descends from KWin, which starts the one keyboard it means to use
started_by_kwin() {
    local pid=$1 kwin
    kwin=$(pgrep -x kwin_wayland | head -1)
    while [ -n "$pid" ] && [ "$pid" -gt 1 ]; do
        [ "$pid" = "$kwin" ] && return 0
        pid=$(ps -o ppid= -p "$pid" 2>/dev/null | tr -d ' ')
    done
    return 1
}

# The desktop file a lookup by name finds first, the way the settings page and Plasma find it
first_desktop_file() {
    local dir dirs
    IFS=: read -r -a dirs <<< "${XDG_DATA_HOME:-$HOME/.local/share}:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
    for dir in "${dirs[@]}"; do
        if [ -e "$dir/applications/$1" ]; then
            echo "$dir/applications/$1"
            return
        fi
    done
}

# Desktop files of every virtual keyboard installed outside the home directory
system_keyboards() {
    local dir
    for dir in /usr/share/applications /usr/local/share/applications /var/lib/flatpak/exports/share/applications; do
        [ -d "$dir" ] || continue
        grep -l -s -x 'X-KDE-Wayland-VirtualKeyboard=true' "$dir"/*.desktop || true
    done
}

# Prints "fork", "stock" or "none" for the virtual keyboard that is running now
running_keyboard() {
    local pid exe
    if flatpak ps --columns=application,branch 2>/dev/null | grep -q -x "${app}[[:space:]]*${branch}"; then
        echo fork
        return
    fi
    for pid in $(pgrep -x plasma-keyboard || true) $(pgrep -x maliit-keyboard || true); do
        exe=$(readlink "/proc/$pid/exe" 2>/dev/null || true)
        case "$exe" in
            /app/*) ;;
            *) echo stock; return ;;
        esac
    done
    echo none
}

status() {
    local setting
    setting=$(kreadconfig6 --file kwinrc --group Wayland --key InputMethod)
    say "KWin starts: ${setting:-(nothing)}"
    [ "$setting" = "$ours" ] || say "  That is not the fork. Run $0 to switch."
    if pgrep -a -x kwin_wayland | grep -q -- "--inputmethod"; then
        say "  But kwin_wayland was started with --inputmethod, which wins over that setting:"
        pgrep -a -x kwin_wayland
    fi

    local pid exe running="" who
    for pid in $(pgrep -x plasma-keyboard || true) $(pgrep -x maliit-keyboard || true); do
        exe=$(readlink "/proc/$pid/exe" 2>/dev/null || true)
        if started_by_kwin "$pid"; then who="started by KWin"; else who="NOT started by KWin, competes with it"; fi
        case "$exe" in
            /app/*) say "Running: the fork (Flatpak), pid $pid, since $(ps -o lstart= -p "$pid"), $who" ;;
            *) say "Running: the STOCK keyboard ($exe), pid $pid, since $(ps -o lstart= -p "$pid"), $who" ;;
        esac
        running="$running $pid"
    done
    [ -n "$running" ] || say "Running: no keyboard"

    local found
    found=$(first_desktop_file "$app.desktop")
    case "$found" in
        "$ours" | "$overrides/$app.desktop") ;;
        *) say "Looking up $app.desktop by name finds $found, so the settings page and Plasma can start the stock keyboard. Run $0 to hide it." ;;
    esac

    if flatpak info --user "$app//$branch" >/dev/null 2>&1; then
        say "Installed fork: $(flatpak run --branch="$branch" --command=plasma-keyboard "$app" --version 2>/dev/null || echo '(could not ask it)')"
        # A keyboard started before the last install keeps running the old build
        local deployed started
        deployed=$(stat -c %Y "$(flatpak info --user --show-location "$app//$branch")")
        for pid in $running; do
            case "$(readlink "/proc/$pid/exe" 2>/dev/null)" in /app/*) ;; *) continue ;; esac
            started=$(( $(date +%s) - $(ps -o etimes= -p "$pid") ))
            if [ "$started" -lt "$deployed" ]; then
                say "  The running fork started before this build was installed, so it is an older build. Run $0 to restart it."
            fi
        done
    else
        say "Installed fork: none"
    fi

    local crashes
    crashes=$(journalctl --user -b 2>/dev/null | grep -c "Input Method crashed" || true)
    [ "$crashes" = 0 ] || say "KWin restarted a crashed keyboard $crashes times since boot: journalctl --user -b | grep -A3 'Input Method crashed'"
}

undo() {
    [ -d "$state" ] || fail "nothing to undo: $state does not exist"
    if [ -f "$state/hidden" ]; then
        while IFS= read -r file; do
            rm -f -- "$file" && say "unhid $(basename "$file")"
        done < "$state/hidden"
    fi
    local previous=""
    [ -f "$state/input-method" ] && previous=$(cat "$state/input-method")
    # If the fork was already set before the first switch, go back to the stock keyboard
    if [ -z "$previous" ] || [ "$previous" = "$ours" ]; then
        previous=""
        [ -f "/usr/share/applications/$app.desktop" ] && previous="/usr/share/applications/$app.desktop"
    fi
    restart_input_method "$previous"
    rm -rf -- "$state"
    say "InputMethod is back to: ${previous:-(not set)}"
}

switch() {
    flatpak info --user "$app//$branch" >/dev/null 2>&1 \
        || fail "the fork's Flatpak ($app//$branch) is not installed for this user. Build it first, see README.md."
    [ -f "$ours" ] || fail "$ours is missing. Run: flatpak make-current --user $app $branch"
    grep -q -- "--branch=$branch" "$ours" \
        || fail "$ours does not start branch $branch. Run: flatpak make-current --user $app $branch"

    # A command line option to KWin wins over kwinrc
    if pgrep -a -x kwin_wayland | grep -q -- "--inputmethod"; then
        say "warning: kwin_wayland was started with --inputmethod, which overrides kwinrc:"
        pgrep -a -x kwin_wayland
    fi

    mkdir -p "$state" "$overrides"
    # Remember the setting from before the first switch, so --undo restores it
    if [ ! -f "$state/input-method" ]; then
        kreadconfig6 --file kwinrc --group Wayland --key InputMethod > "$state/input-method"
    fi

    # A user file with Hidden=true hides a desktop file of the same name from System Settings and from
    # anything that starts it by name. The stock keyboard's file has the same name as the Flatpak's, and
    # where /usr/share comes first in XDG_DATA_DIRS a lookup by that name finds the stock one. Hiding the
    # name too stops the settings page from offering it and Plasma from starting it; KWin keeps opening
    # the Flatpak's file by its full path.
    local file name
    while IFS= read -r file; do
        name=$(basename "$file")
        [ -e "$overrides/$name" ] && continue
        printf '[Desktop Entry]\nHidden=true\n' > "$overrides/$name"
        echo "$overrides/$name" >> "$state/hidden"
        say "hid $name"
    done < <(system_keyboards)

    # Mode 0 never shows the keyboard
    local mode
    mode=$(kreadconfig6 --file kwinrc --group Wayland --key VirtualKeyboardMode)
    if [ "$mode" = 0 ] || [ "$(kreadconfig6 --file kwinrc --group Wayland --key VirtualKeyboardEnabled)" = false ]; then
        kwriteconfig6 --file kwinrc --group Wayland --key VirtualKeyboardMode 1
        say "the virtual keyboard was switched off in KWin; it now shows for touch input"
    fi

    restart_input_method "$ours"
    say "InputMethod is now: $ours"

    local now
    for _ in 1 2 3 4 5 6 7 8 9 10; do
        now=$(running_keyboard)
        [ "$now" = fork ] && break
        sleep 1
    done
    case "$now" in
        fork) say "OK: the fork is running ($(flatpak ps --columns=application,branch | grep "^$app" | head -1))." ;;
        stock) fail "the stock keyboard is still running. Log out and back in, then run this script again." ;;
        none) fail "no keyboard is running. Check: journalctl --user -b | grep -i -E 'plasma-keyboard|input method'" ;;
    esac
}

case "${1:-}" in
    "") switch ;;
    --status) status ;;
    --undo) undo ;;
    *) fail "usage: $0 [--status | --undo]" ;;
esac
