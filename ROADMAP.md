<!--
  - SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
  - SPDX-License-Identifier: CC0-1.0
-->

# Fork roadmap

Plans for the `deck-features` branch, in the order they are meant to land. What already works is in the
[README](README.md#fork-features). Each milestone is built test first, tried in the porygon test desktop (a
KDE session under a real KWin), and ends with a product test on the Steam Deck.

## Where things stand

The features in the README's table are done and passed product test A in the browser test desktop.
Product test B, on the Deck with the Flatpak build, passed apart from the theme: the keyboard stayed light after
the desktop switched to dark. Build `e4b1ccb` fixes that: on the Deck the running keyboard followed a switch to light and back. Still to check by hand:

1. Switch the desktop between light and dark while the keyboard is open, and again while it is closed.
2. The keyboard should match each time, without logging out.

## 1. Dock at the top, left or right

**Goal:** besides the bottom dock and floating, dock the keyboard against the top, left or right edge, with
windows making room for it.

**Approach:** the input-method protocol only docks at the bottom (`set_toplevel` at `position_center_bottom`),
and floating is a full-screen panel that is positioned with an input region mask. Docking at other edges takes a
layer-shell surface anchored to that edge, with an exclusive zone so windows make room. This is how the Plasma
panel works too.

**Unknowns:**
- The KDE Flatpak runtime has no layer-shell-qt, so the manifest has to build it, as deckhud's does.
- A Qt window can't change between an input panel and a layer-shell surface, so changing the dock edge
  recreates the window. QtVirtualKeyboard's state has to survive that.
- Does KWin still route input-method events to a keyboard drawn on a layer-shell surface? Try this first, with a
  throwaway build.

**Done when:** the settings and the Move overlay offer bottom, top, left, right and floating. Each edge
holds after a restart, and maximized windows don't go under the keyboard.

## 2. Mode A: share the screen with the window being typed in

**Goal:** a placement mode that keeps the keyboard off the window being typed in. It picks the edge or quadrant
that leaves that window the most room. The current floating mode stays as Mode B.

**Approach:** depends on milestone 1. KWin already moves the focused window above a keyboard docked at the
bottom. For the other edges, choose from:
- the text field's cursor rectangle, which `text-input` gives relative to the window;
- the screen's free area.

Prefer docking opposite the cursor, and add a hysteresis so the keyboard doesn't jump while typing.

**Unknowns:** the input method gets no global window geometry from KWin. If the cursor rectangle isn't
enough, a small KWin script can report the active window's frame over D-Bus.

**Done when:** typing near the bottom of a window docks the keyboard at the top and the reverse, on the Deck in
landscape, without flicker between keystrokes.

## 3. Split keyboard, then tilt

**Goal:** two halves at the left and right edges for thumbs on a landscape screen, like the Deck's. Each half can
then tilt inward so wrists stay straight.

**Approach:**
- Reuse the floating surface. Its input region becomes two rectangles, so taps in the middle reach the
  window behind.
- A custom layout marks where the rows split. Until then, use a default split of the QWERTY rows, with
  space, backspace and enter repeated on both sides.
- Each half is an item rotated by the tilt angle. QtQuick maps touches through the rotation, but gesture
  typing has to map swipe points into each half's coordinates, and a swipe may cross from one half to the other.

**Unknowns:** the halves are drawn through QtVirtualKeyboard's style, which assumes one rectangle of keys.
That may need a custom keyboard item instead of the style's layout.

**Done when:** a split setting with width and tilt sliders, gesture typing across halves, and the input
region checked under KWin in the mock compositor test.

## 4. Rotate the screen for portrait typing

**Goal:** hold the Deck sideways and type in portrait.

**Approach, in two steps:**
1. A command that rotates the screen, for example `kscreen-doctor output.eDP.rotation.left`, made available as a
   global shortcut and as a toolbar button. The keyboard has to lay out again for the new screen size.
2. Automatic rotation from the Deck's gyro. KWin rotates by itself only from iio-sensor-proxy, and the Deck's
   IMU belongs to the controller (hidraw 28DE:1205, the same device as haptics). A small user service would read
   the accelerometer reports and call the same rotation command, with a delay so it doesn't flip by accident.

**Unknowns:** when Steam is running it reads the controller too. Check whether IMU reports still arrive
through hidraw then, and the report layout.

**Done when:** step 1 works from the shortcut on the Deck. Step 2 only if the IMU is readable alongside Steam.

## 5. Graphical layout editor

**Goal:** edit custom layouts by hand instead of in JSON.

**Approach:** an editing mode, opened from the settings page or the Move overlay, that shows the layout's rows.
It lets you:
- add, remove and reorder keys;
- change key widths by dragging;
- pick special keys (Tab, Esc, either Shift, arrows, Enter);
- choose where the split goes (milestone 3).

It writes the same JSON file the custom layout setting reads, and the keyboard already reloads that file on save.

**Done when:** a layout made in the editor round-trips through the JSON file unchanged, and an invalid edit
can't leave the keyboard without letters.

## Smaller items

- Report upstream that KWin 6.7 doesn't keep input-method protocols for the keyboard it started. They are
  missing from `interfacesBlackList`, so any client can act as the keyboard. See
  [tools/deck/use-fork-keyboard.sh](tools/deck/use-fork-keyboard.sh) for how the fork copes.
- porygon's test desktop now has Flatpak: run the Flatpak build as KWin's input method there. Sandbox behaviour,
  such as the color scheme, can then be tested without the Deck.
- Find out what made Plasma's system tray start the stock keyboard by name once on the Deck. It's harmless now,
  because only the keyboard KWin starts stays running.
