/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.keyboard

// Toolbar above the keys with layers and keyboard tools, shown while there are no word candidates.

RowLayout {
    id: root

    property var keyboardController: null
    property var inputMethodConnection: null
    property bool voiceInputAvailable: false
    property bool voiceInputActive: false

    signal adjustRequested()
    signal settingsRequested()
    signal voiceInputRequested()

    readonly property string currentLayer: keyboardController ? keyboardController.layer : ""

    spacing: 0

    ToolbarButton {
        iconName: "smiley-symbolic"
        checked: root.currentLayer === "emoji"
        onClicked: root.keyboardController.toggleLayer("emoji")
    }

    ToolbarButton {
        iconName: "edit-select-text-symbolic"
        checked: root.currentLayer === "editing"
        onClicked: root.keyboardController.toggleLayer("editing")
    }

    ToolbarButton {
        iconName: "utilities-terminal-symbolic"
        checked: root.currentLayer === "terminal"
        onClicked: root.keyboardController.toggleLayer("terminal")
    }

    ToolbarButton {
        visible: root.voiceInputAvailable
        iconName: root.voiceInputActive ? "media-record-symbolic" : "audio-input-microphone-symbolic"
        checked: root.voiceInputActive
        onClicked: root.voiceInputRequested()
    }

    Item {
        Layout.fillWidth: true
    }

    ToolbarButton {
        iconName: "transform-move-symbolic"
        onClicked: root.adjustRequested()
    }

    ToolbarButton {
        iconName: root.inputMethodConnection && root.inputMethodConnection.pinned ? "window-unpin-symbolic" : "window-pin-symbolic"
        checked: !!root.inputMethodConnection && root.inputMethodConnection.pinned
        onClicked: root.inputMethodConnection.pinned = !root.inputMethodConnection.pinned
    }

    ToolbarButton {
        iconName: "configure-symbolic"
        onClicked: root.settingsRequested()
    }
}
