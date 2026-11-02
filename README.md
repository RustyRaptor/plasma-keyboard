<!--
  - SPDX-FileCopyrightText: None
  - SPDX-License-Identifier: CC0-1.0
-->

# Plasma Keyboard

> **This is a fork** ([RustyRaptor/plasma-keyboard](https://github.com/RustyRaptor/plasma-keyboard), branch `deck-features`)
> that adds features aimed at the Steam Deck and other touch devices. See [Fork features](#fork-features) below.
> Upstream lives at [invent.kde.org/plasma/plasma-keyboard](https://invent.kde.org/plasma/plasma-keyboard).


The plasma-keyboard is a virtual keyboard and input engine designed to integrate in Plasma.

It uses the input-method-v1 Wayland protocol to communicate with the compositor to function as an input method. It also can leverage KWin's fake-input protocol in order to emulate keyboard keys (ex. Meta, Ctrl).

#### Install using the flatpak nightly repository

Flatpak manifest: [org.kde.plasma.keyboard.flatpakref](https://cdn.kde.org/flatpak/plasma-keyboard-nightly/org.kde.plasma.keyboard.flatpakref)

See also: [userbase.kde.org/Tutorials/Flatpak#Nightly_KDE_apps](https://userbase.kde.org/Tutorials/Flatpak#Nightly_KDE_apps)

## Fork features

| Feature | How to use it | Settings |
|---|---|---|
| Padding around the keys | A strip above the keys swallows taps that land slightly too high, so they don't focus the window behind and close the keyboard. A floating keyboard can also have padding beside the keys, for thumbs holding the device | *Padding above keys*, *Padding beside keys* |
| Size and position | Drag the handle bar above the keys to move a floating keyboard. Toolbar → move button: drag the keyboard (floating mode), resize it with the corner handle, dock/float, reset | *Panel* section |
| Typing preview | A strip above the keys shows the line being typed around the cursor, so a covered text field can still be followed. Never shown for passwords | *Preview* |
| Custom layout | Your own rows of keys in a JSON file replace the letters: choose which keys exist (Tab, Esc, either Shift, arrows, Enter...) and how wide they are. See below | *Custom layout* |
| Transparency | Make the keyboard see-through to watch what's underneath while typing | *Transparency*, *Opacity* |
| Pin | Toolbar → pin button keeps the keyboard up when the text field goes away. Without a text field it types by emulating keys (US layout) | — |
| Flicker fix | Hiding waits a moment, so web pages that briefly recreate their text field no longer make the keyboard flash | *Hide delay* |
| Word suggestions | Shown above the keys while typing; tap one to replace the word. Learns the words you type (new words after two uses) | *Suggestions* |
| Local language model | Its prediction for the current or next word leads the suggestions. Supports llama.cpp, Ollama and OpenAI compatible servers | *Language model* |
| Voice typing | Toolbar → microphone, speak, tap again to stop. Transcribed by a whisper.cpp or OpenAI compatible server | *Voice Input* |
| Editing layer | Toolbar → select-text button: select all, cut, copy, paste, undo/redo, word and line navigation, a selection toggle and a cursor pad | — |
| Gesture typing | Swipe across the letters of a word without lifting your finger | *Gestures* |
| Steam Deck haptics | Ticks the trackpads (or rumbles) on key presses, through the controller's hidraw device | *Haptics* |
| Emoji layer | Toolbar → smiley button. Long press for skin tones. Recently used emoji sit in a row above the groups | — |
| Spacebar cursor | Swipe sideways on the spacebar to move the cursor; hold shift first to select | *Gestures* |
| Terminal layer | Toolbar → terminal button: Esc, Tab, sticky Ctrl/Alt, ^C ^D ^Z ^L ^R ^A ^E, arrows, shell symbols | — |

Nothing typed is sent anywhere unless you configure a language model or speech server, and suggestions are
never computed (or learned) in password, URL, e-mail and number fields.

### Installing the fork on a Steam Deck

The fork builds as a Flatpak (branch `deck`) so SteamOS stays untouched:

```bash
cd flatpak
flatpak run org.flatpak.Builder --user --install --force-clean build-dir org.kde.plasma.keyboard.json

# Use it as the input method
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod \
    "$HOME/.local/share/flatpak/exports/share/applications/org.kde.plasma.keyboard.desktop"
```

To go back to the stock keyboard, set `InputMethod` back to `/usr/share/applications/org.kde.plasma.keyboard.desktop`.
Settings live in `~/.var/app/org.kde.plasma.keyboard/config/plasmakeyboardrc` and open from the toolbar's settings button.

### Custom layouts

Point *Custom layout* in the settings at a JSON file to replace the letters of the keyboard layout with your
own rows. The symbol and number layers stay. Changes to the file apply as soon as it's saved.

```json
{
    "rows": [
        ["escape", {"chars": "qwertyuiop"}, {"key": "backspace", "weight": 2}],
        [{"key": "tab", "weight": 1.5}, {"chars": "asdfghjkl'"}, {"key": "enter", "weight": 1.5}],
        [{"key": "shift", "weight": 2}, {"chars": "zxcvbnm,.?"}, "shift"],
        ["symbols", "ctrl", "alt", {"key": "space", "weight": 5}, "arrows", "language", "hide"]
    ]
}
```

| Entry | Keys |
|---|---|
| `{"chars": "abc"}` | One key per character. Long press offers accented letters, or your own with `"alternatives": {"a": "aäå"}` |
| `"name"` or `{"key": "name"}` | `escape`, `tab`, `backspace`, `enter`, `shift`, `space`, `symbols`, `ctrl`, `alt`, `meta`, `left`, `right`, `up`, `down`, `language`, `hide`, and `gap` for empty space |
| `"arrows"` | Left, up over down, and right, three keys wide |
| `"weight": 1.5` | Width in normal keys, for any entry except `arrows` |

Keys stay square, so the widest row sets the keyboard's width. [examples/custom-layouts](examples/custom-layouts)
has a copy of the desktop layout and a minimal one to start from. Mistakes are logged by the keyboard, which keeps
the normal letters until the file is fixed.

### Local model servers

Word prediction only needs a small model; it asks for a handful of tokens at temperature 0.

```bash
# llama.cpp (Language model: llama.cpp server, address http://127.0.0.1:8080)
llama-server -m qwen2.5-0.5b-q8_0.gguf --port 8080

# Ollama (Language model: Ollama, address http://127.0.0.1:11434, model qwen3:0.6b)
ollama pull qwen3:0.6b

# whisper.cpp (Speech recognition: whisper.cpp server, address http://127.0.0.1:8081)
whisper-server -m ggml-base.en.bin --port 8081
```

### Development

Everything new is covered by unit tests in `autotests/`, and the mock compositor test drives the real
keyboard end to end (layers, swiping, pinning...). Set `PLASMA_KEYBOARD_SCREENSHOT_DIR` to make it save
screenshots of what it tests.

#### Repository structure

Locations in the repo:

* [layouts](layouts) - Keyboard layouts
* [kcm](kcm) - Settings module
* [src](src) - Wayland input method (application), virtual keyboard, diacritics, etc.
* [src/overlay](/src/overlay) - Overlay components for physical keyboard (diacritics)
* [virtualkeyboard](virtualkeyboard) - Input engine and keyboard framework

#### Layouts

The keyboard layouts are located in the [layouts](/layouts) folder.

See the existing layouts there for examples on creating layout packages. They are installed (roughly) to `/usr/share/plasma/keyboard/keyboardpackages` or `~/.local/share/plasma/keyboard/keyboardpackages/`.

See the component library for making layouts for documentation at [virtualkeyboard/components](/virtualkeyboard/components).

#### Text composers

A default text composer is provided that emits keys directly to the input engine (see [directtextcomposer](virtualkeyboard/textcomposers/directtextcomposer.h)) which works for most languages.

Some languages require some extra processing in preedit to form words, and so have their own text composers:

Text composers are located in [virtualkeyboard/textcomposers](/virtualkeyboard/textcomposers).

#### Troubleshooting

KWin by default only shows the keyboard when a text field is interacted with by touch. Set `KWIN_IM_SHOW_ALWAYS=1` when starting KWin (or the login session) in order to force the keyboard to always pop up.

# Development

Recommended methods for development are to either use
[KDE Linux](https://linux.kde.org/) (as your OS or in a VM), or to build the
Flatpak version of plasma-keyboard.

#### KDE Linux (recommended)

Follow the
[instructions](https://linux.kde.org/docs/kde-dev/#build-kde-software-thats-shipped-on-the-base-image)
to set up a development environment for KDE software.

Build plasma-keyboard once to clone the source locally:

```bash
kde-builder plasma-keyboard
```

After making changes to the code the workflow is: rebuild, refresh sysext, and restart plasma-keyboard. This
script can be used to do that:

```bash
#!/usr/bin/env bash

set -e

# Rebuild plasma-keyboard and refresh the sysext
kde-builder --no-src plasma-keyboard && systemctl --user daemon-reload && sudo systemd-sysext refresh --always-refresh=yes && systemctl restart --user plasma-plasmashell.service

# Disable plasma-keyboard
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod ''

# Enable plasma-keyboard with the newly built changes
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod '/usr/share/applications/org.kde.plasma.keyboard.desktop'
```

#### kde-builder (for most distributions)

For most distributions where you have kde-builder setup, you can test plasma-keyboard in a nested Plasma session.

Build plasma-keyboard:

```bash
$ kde-builder plasma-keyboard # For first run
$ kde-builder plasma-keyboard -DS # To build without dependencies
```

Ensure KWin uses your development version of plasma-keyboard by editing `~/.config/kwinrc`:

```
~/.config/kwinrc

[Wayland]
InputMethod=~/kde/usr/share/applications/org.kde.plasma.keyboard.desktop
```

Run Plasma in a nested window:

```bash
$ KWIN_IM_SHOW_ALWAYS=1 dbus-run-session kwin_wayland "plasmashell"
```

In order to test keyboard navigation, pass an extra flag to KWin so that it would forward keyboard events:

```bash
$ KWIN_DEVICE_SKIPS_INPUT_METHOD="_none_" dbus-run-session kwin_wayland "plasmashell"
```

#### Flatpak

It is also possible to build plasma-keyboard as a Flatpak. This is the
recommended method for development on distributions other than KDE Linux, as we
don't want to install the development version of plasma-keyboard on a
traditional Linux distribution (which may break the system).

Clone the repository and make changes to the code, then build and install the
development version of the Flatpak with your changes by running the following
from the repository root (or save as a script and run it):

```bash
#!/usr/bin/env bash

# Build and install the flatpak
flatpak-builder --user --install --force-clean build-flatpak .flatpak-manifest.json

# Disable plasma-keyboard if it is already running
killall plasma-keyboard; kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod ''

# Enable plasma-keyboard with the newly built changes
kwriteconfig6 --notify --file kwinrc --group Wayland --key InputMethod '$HOME/.local/share/flatpak/exports/share/applications/org.kde.plasma.keyboard.desktop'

```

#### Building from source manually

```sh
mkdir build && cd build
cmake ..
make && make install
```

#### Troubleshooting

Join the [KDE Matrix chat](https://community.kde.org/Matrix) so we can help you
get started with development! We have a room specifically for input handling:
[#kde-input:kde.org](https://matrix.to/#/#kde-input:kde.org)

