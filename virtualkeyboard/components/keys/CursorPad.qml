// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick

import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard

// Touchpad-like area that moves the text cursor in two dimensions while dragging on it.

AbstractKey {
    id: root

    /**
     * Whether moving the cursor extends the selection.
     */
    property bool selecting: false

    readonly property alias dragging: tracker.active

    iconName: "transform-move-symbolic"
    secondaryStyle: true
    showPreview: false
    highlighted: tracker.active

    CursorDragTracker {
        id: tracker
        activationDistance: Math.round(12 * BreezeConstants.scaleHint)
        stepSize: Math.round(36 * BreezeConstants.scaleHint)
        verticalStepSize: Math.round(80 * BreezeConstants.scaleHint)
    }

    KeyMouseArea {
        keyItem: root

        onPressStarted: (point) => tracker.begin(Qt.point(point.x, point.y))

        onPositionChanged: (point) => {
            const steps = tracker.update(Qt.point(point.x, point.y));
            if ((steps.x !== 0 || steps.y !== 0) && VirtualKeyboard.inputEngine) {
                Feedback.play(Feedback.SelectionChange);
                VirtualKeyboard.inputEngine.moveCursor(steps.x, steps.y, root.selecting);
            }
        }

        onReleaseFinished: tracker.end()
        onCancelFinished: tracker.end()
    }
}
