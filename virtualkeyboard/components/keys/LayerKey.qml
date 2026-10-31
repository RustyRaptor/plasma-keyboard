// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick

import org.kde.plasma.keyboard.virtualkeyboard

// Action key that switches to a keyboard layer, or back to the regular layout when layer is empty.

ActionKey {
    id: root

    /**
     * The layer to show, such as "editing" or "terminal". Empty returns to the regular layout.
     */
    property string targetLayer: ""

    readonly property bool layerActive: !!VirtualKeyboard.keyboardController && VirtualKeyboard.keyboardController.layer === root.targetLayer

    displayText: targetLayer.length === 0 ? "ABC" : ""
    functionKey: true
    secondaryStyle: true
    noModifier: true
    highlighted: targetLayer.length > 0 && layerActive
    textPixelSize: 30 * scaleHint

    onClicked: {
        const controller = VirtualKeyboard.keyboardController;
        if (!controller) {
            return;
        }
        if (root.targetLayer.length === 0) {
            controller.symbolMode = false;
            controller.layer = "";
        } else {
            controller.toggleLayer(root.targetLayer);
        }
    }
}
