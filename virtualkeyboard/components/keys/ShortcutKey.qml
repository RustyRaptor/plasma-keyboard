// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick

import org.kde.plasma.keyboard.virtualkeyboard

// Key that sends a key combination, such as Ctrl+C.

AbstractKey {
    id: root

    /**
     * Key sent while the modifiers are held.
     */
    property int key: Qt.Key_unknown

    /**
     * Modifier keys held while sending the key.
     */
    property var modifiers: []

    functionKey: true
    secondaryStyle: true
    noModifier: true
    showPreview: false
    textPixelSize: 30 * scaleHint

    function sendShortcut() {
        const engine = VirtualKeyboard.inputEngine;
        if (engine) {
            engine.sendShortcut(root.key, root.modifiers);
        }
    }

    function trigger() {
        if (!enabled) {
            return;
        }
        sendShortcut();
        clicked();
    }

    KeyMouseArea {
        keyItem: root
        repeatEnabled: root.repeat

        onRepeatTriggered: root.sendShortcut()

        onReleaseFinished: {
            if (!root.repeat) {
                root.sendShortcut();
            }
            root.clicked();
        }
    }
}
