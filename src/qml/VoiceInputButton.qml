/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick

import org.kde.plasma.keyboard

// Microphone button that starts and stops voice typing, shown while it's set up.

ToolbarButton {
    id: root

    property var voiceInput: null

    /**
     * Emitted after the button started or stopped voice typing.
     */
    signal voiceInputToggled()

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
    onClicked: {
        root.voiceInput.toggle();
        root.voiceInputToggled();
    }
}
