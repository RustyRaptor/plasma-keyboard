/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick

// DragHandler for moving or resizing the keyboard panel, which reports how far it was dragged once released.

DragHandler {
    id: root

    /**
     * How far the current drag has gone, kept until the drag ends.
     */
    property point translation: Qt.point(0, 0)

    signal finished(point offset)

    target: null

    // activeTranslation is already back to zero when active turns false, so the last one is kept
    onActiveTranslationChanged: {
        if (root.active) {
            root.translation = Qt.point(root.activeTranslation.x, root.activeTranslation.y);
        }
    }
    onActiveChanged: {
        if (!root.active) {
            root.finished(root.translation);
            root.translation = Qt.point(0, 0);
        }
    }
}
