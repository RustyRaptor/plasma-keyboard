/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Toolbar above the keys with layers and keyboard tools, shown while there are no word candidates.

RowLayout {
    id: root

    property var keyboardController: null
    property var inputMethodConnection: null
    readonly property var voiceInput: inputMethodConnection ? inputMethodConnection.voiceInput : null

    signal adjustRequested()
    signal settingsRequested()

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
        visible: !!root.voiceInput && root.voiceInput.available
        iconName: {
            if (!root.voiceInput) {
                return "";
            }
            switch (root.voiceInput.state) {
            case VoiceInput.Recording:
                return "media-playback-stop-symbolic";
            case VoiceInput.Transcribing:
                return "process-working-symbolic";
            default:
                return "audio-input-microphone-symbolic";
            }
        }
        checked: !!root.voiceInput && root.voiceInput.state !== VoiceInput.Idle
        onClicked: root.voiceInput.toggle()
    }

    QQC2.Label {
        visible: !!root.voiceInput && root.voiceInput.state === VoiceInput.Recording
        text: i18n("Listening…")
        color: BreezeConstants.selectionListTextColor
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
