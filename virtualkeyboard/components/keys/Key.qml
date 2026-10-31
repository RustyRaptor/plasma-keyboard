// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick

import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard

// Standard key that sends text/key clicks through the input method.

AbstractKey {
    id: root

    /**
     * Logical key code sent to the input engine when the key is activated.
     */
    property int key: Qt.Key_unknown

    property string commitText: root.text

    /**
     * Whether swiping sideways across the key moves the text cursor instead of typing.
     * Holding shift while swiping selects text.
     */
    property bool cursorDragEnabled: false
    readonly property alias cursorDragActive: cursorDragTracker.active
    property bool __cursorDragSelects: false
    property bool noKeyEvent: key === Qt.Key_unknown && commitText.length === 0
    readonly property bool alternativeKeysActive: {
        const popup = VirtualKeyboard.alternativeKeysPopup;
        return !!(popup && popup.active && popup.ownerKey === root);
    }

    function trigger() {
        if (!enabled) {
            return;
        }
        if (!noKeyEvent) {
            commitKeyText(root.commitText);
        }
        clicked();
    }

    function commitAlternativeText(text) {
        commitKeyText(text);
    }

    function commitKeyText(text) {
        const engine = VirtualKeyboard.inputEngine;
        if (engine) {
            engine.sendTextComposerKey(root.key, text);
        }
    }

    CursorDragTracker {
        id: cursorDragTracker
        activationDistance: Math.round(40 * BreezeConstants.scaleHint)
        stepSize: Math.round(30 * BreezeConstants.scaleHint)

        onActiveChanged: {
            if (!active) {
                return;
            }
            const characterPopup = VirtualKeyboard.characterPopup;
            if (characterPopup) {
                characterPopup.closeForKey(root);
            }
            keyMouseArea.stopRepeat();
        }
    }

    KeyMouseArea {
        id: keyMouseArea
        keyItem: root
        repeatEnabled: root.repeat && !root.noKeyEvent
        pressAndHoldInterval: 500

        onRepeatTriggered: root.commitKeyText(root.commitText)

        onPressStarted: (mouse) => {
            if (root.cursorDragEnabled) {
                const engine = VirtualKeyboard.inputEngine;
                root.__cursorDragSelects = !!engine && engine.shiftActive;
                cursorDragTracker.begin(Qt.point(mouse.x, mouse.y));
            }

            const popup = VirtualKeyboard.alternativeKeysPopup;
            if (popup && popup.active && popup.ownerKey !== root) {
                popup.close();
            }

            const characterPopup = VirtualKeyboard.characterPopup;
            if (characterPopup) {
                characterPopup.openForKey(root);
            }
        }

        onPressAndHold: (mouse) => {
            const popup = VirtualKeyboard.alternativeKeysPopup;
            if (!popup || root.effectiveAlternativeKeys.length === 0) {
                return;
            }

            if (popup.openForKey(root)) {
                const characterPopup = VirtualKeyboard.characterPopup;
                if (characterPopup) {
                    characterPopup.closeForKey(root);
                }
                stopRepeat();
            }
        }

        onPositionChanged: (mouse) => {
            if (root.cursorDragEnabled) {
                const steps = cursorDragTracker.update(Qt.point(mouse.x, mouse.y));
                if (steps.x !== 0 && VirtualKeyboard.inputEngine) {
                    Feedback.play(Feedback.SelectionChange);
                    VirtualKeyboard.inputEngine.moveCursor(steps.x, 0, root.__cursorDragSelects);
                }
                if (cursorDragTracker.active) {
                    return;
                }
            }

            const popup = VirtualKeyboard.alternativeKeysPopup;
            if (pressed && popup && popup.active && popup.ownerKey === root) {
                popup.moveFromKey(root, mouse.x);
            }
        }

        onReleaseFinished: (mouse) => {
            if (cursorDragTracker.active) {
                cursorDragTracker.end();
                const engine = VirtualKeyboard.inputEngine;
                if (root.__cursorDragSelects && engine && !engine.capsLockActive) {
                    engine.shiftActive = false;
                }
                return;
            }
            cursorDragTracker.end();

            const characterPopup = VirtualKeyboard.characterPopup;
            if (characterPopup) {
                characterPopup.closeForKey(root);
            }

            const popup = VirtualKeyboard.alternativeKeysPopup;
            if (popup && popup.active && popup.ownerKey === root) {
                popup.commitCurrent();
                return;
            }

            if (root.noKeyEvent) {
                root.clicked();
                return;
            }

            if (!root.repeat) {
                root.commitKeyText(root.commitText);
            }
            root.clicked();
        }

        onCancelFinished: {
            cursorDragTracker.end();

            const characterPopup = VirtualKeyboard.characterPopup;
            if (characterPopup) {
                characterPopup.closeForKey(root);
            }

            const popup = VirtualKeyboard.alternativeKeysPopup;
            if (popup && popup.active && popup.ownerKey === root) {
                popup.close();
                return;
            }
        }
    }
}
