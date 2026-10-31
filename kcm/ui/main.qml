/*
    SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: root

    Kirigami.FormLayout {
        id: formLayout

        width: flickable.width

        QQC2.Button {
            Kirigami.FormData.label: i18n("Keyboard layouts:")
            text: i18n("Configure…")
            icon.name: "languages"
            onClicked: kcm.push("KeyboardLayoutsPage.qml")
        }

        QQC2.CheckBox {
            id: soundsEnabled
            Kirigami.FormData.label: i18n("Key press feedback:")
            text: i18n("Sound")

            checked: kcm.plasmaKeyboardSettings.soundEnabled
            onCheckedChanged: kcm.plasmaKeyboardSettings.soundEnabled = checked
        }

        QQC2.CheckBox {
            id: vibrationEnabled
            text: i18n("Vibration")

            checked: kcm.plasmaKeyboardSettings.vibrationEnabled
            onCheckedChanged: kcm.plasmaKeyboardSettings.vibrationEnabled = checked
        }

        QQC2.CheckBox {
            id: characterPopupEnabled
            text: i18n("Character popup")

            checked: kcm.plasmaKeyboardSettings.characterPopupEnabled
            onCheckedChanged: kcm.plasmaKeyboardSettings.characterPopupEnabled = checked
        }

        QQC2.CheckBox {
            id: keyboardNavigationEnabled
            Kirigami.FormData.label: i18n("General:")
            text: i18n("Keyboard navigation")

            checked: kcm.plasmaKeyboardSettings.keyboardNavigationEnabled
            onCheckedChanged: kcm.plasmaKeyboardSettings.keyboardNavigationEnabled = checked
        }

        QQC2.CheckBox {
            id: autoCapitalizationEnabled
            text: i18n("Auto-capitalization")

            checked: kcm.plasmaKeyboardSettings.autoCapitalizationEnabled
            onCheckedChanged: kcm.plasmaKeyboardSettings.autoCapitalizationEnabled = checked
        }

        QQC2.CheckBox {
            id: diacriticsCheckbox
            Kirigami.FormData.label: i18n("Alternate characters:")
            text: i18n("Show popup when holding a key")

            checked: kcm.plasmaKeyboardSettings.diacriticsPopupEnabled
            onCheckedChanged: kcm.plasmaKeyboardSettings.diacriticsPopupEnabled = checked
        }

        QQC2.SpinBox {
            id: diacriticsDelaySpinBox
            Kirigami.FormData.label: i18n("Hold delay:")
            from: 100
            to: 1500
            stepSize: 50

            enabled: diacriticsCheckbox.checked
            value: kcm.plasmaKeyboardSettings.diacriticsHoldThresholdMs

            // Include the `milliseconds` suffix in the spinbox instead of the label
            textFromValue: function (value) {
                return value + " " + i18n("milliseconds");
            }

            // Parse the integer value from the spinbox text, ignoring the suffix
            valueFromText: function (text) {
                let number = parseInt(text);
                if (isNaN(number)) {
                    return kcm.plasmaKeyboardSettings.diacriticsHoldThresholdMs; // Fallback to current value if parsing fails
                }
                return number;
            }

            onValueChanged: kcm.plasmaKeyboardSettings.diacriticsHoldThresholdMs = value
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Panel")
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Padding above keys:")
            from: 0
            to: 200
            stepSize: 4
            value: kcm.plasmaKeyboardSettings.panelTopPadding
            textFromValue: (value) => i18np("%1 pixel", "%1 pixels", value)
            valueFromText: (text) => parseInt(text) || 0
            onValueModified: kcm.plasmaKeyboardSettings.panelTopPadding = value
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18n("Position:")
            text: i18n("Float instead of docking to the bottom")
            checked: kcm.plasmaKeyboardSettings.panelFloating
            onToggled: kcm.plasmaKeyboardSettings.panelFloating = checked
        }

        QQC2.Slider {
            Kirigami.FormData.label: i18n("Height:")
            from: 0.4
            to: 2
            stepSize: 0.05
            value: kcm.plasmaKeyboardSettings.panelHeightScale
            onMoved: kcm.plasmaKeyboardSettings.panelHeightScale = value
        }

        QQC2.Slider {
            Kirigami.FormData.label: i18n("Floating width:")
            enabled: kcm.plasmaKeyboardSettings.panelFloating
            from: 0.25
            to: 1
            stepSize: 0.05
            value: kcm.plasmaKeyboardSettings.panelWidthFraction
            onMoved: kcm.plasmaKeyboardSettings.panelWidthFraction = value
        }

        QQC2.SpinBox {
            Kirigami.FormData.label: i18n("Hide delay:")
            from: 0
            to: 3000
            stepSize: 100
            value: kcm.plasmaKeyboardSettings.panelHideDelayMs
            textFromValue: (value) => i18n("%1 milliseconds", value)
            valueFromText: (text) => parseInt(text) || 0
            onValueModified: kcm.plasmaKeyboardSettings.panelHideDelayMs = value
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18n("Toolbar:")
            text: i18n("Show layers and tools above the keys")
            checked: kcm.plasmaKeyboardSettings.toolbarEnabled
            onToggled: kcm.plasmaKeyboardSettings.toolbarEnabled = checked
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Typing")
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18n("Gestures:")
            text: i18n("Swipe across letters to type words")
            checked: kcm.plasmaKeyboardSettings.gestureTypingEnabled
            onToggled: kcm.plasmaKeyboardSettings.gestureTypingEnabled = checked
        }

        QQC2.CheckBox {
            text: i18n("Swipe on the spacebar to move the cursor")
            checked: kcm.plasmaKeyboardSettings.spacebarCursorEnabled
            onToggled: kcm.plasmaKeyboardSettings.spacebarCursorEnabled = checked
        }

        QQC2.CheckBox {
            Kirigami.FormData.label: i18n("Suggestions:")
            text: i18n("Suggest words while typing")
            checked: kcm.plasmaKeyboardSettings.suggestionsEnabled
            onToggled: kcm.plasmaKeyboardSettings.suggestionsEnabled = checked
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Language model:")
            enabled: kcm.plasmaKeyboardSettings.suggestionsEnabled
            model: [i18n("None"), i18n("llama.cpp server"), i18n("Ollama"), i18n("OpenAI compatible server")]
            currentIndex: kcm.plasmaKeyboardSettings.suggestionModelApi
            onActivated: (index) => kcm.plasmaKeyboardSettings.suggestionModelApi = index
        }

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Server address:")
            visible: kcm.plasmaKeyboardSettings.suggestionModelApi !== 0
            placeholderText: "http://127.0.0.1:8080"
            text: kcm.plasmaKeyboardSettings.suggestionModelEndpoint
            onEditingFinished: kcm.plasmaKeyboardSettings.suggestionModelEndpoint = text
        }

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Model:")
            visible: kcm.plasmaKeyboardSettings.suggestionModelApi >= 2
            placeholderText: i18nc("@info:placeholder example model name", "e.g. qwen3:0.6b")
            text: kcm.plasmaKeyboardSettings.suggestionModelName
            onEditingFinished: kcm.plasmaKeyboardSettings.suggestionModelName = text
        }

        Kirigami.PasswordField {
            Kirigami.FormData.label: i18n("API key:")
            visible: kcm.plasmaKeyboardSettings.suggestionModelApi !== 0
            placeholderText: i18nc("@info:placeholder", "Optional")
            text: kcm.plasmaKeyboardSettings.suggestionModelApiKey
            onEditingFinished: kcm.plasmaKeyboardSettings.suggestionModelApiKey = text
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Voice Input")
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Speech recognition:")
            model: [i18n("Disabled"), i18n("whisper.cpp server"), i18n("OpenAI compatible server")]
            currentIndex: kcm.plasmaKeyboardSettings.voiceInputApi
            onActivated: (index) => kcm.plasmaKeyboardSettings.voiceInputApi = index
        }

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Server address:")
            visible: kcm.plasmaKeyboardSettings.voiceInputApi !== 0
            placeholderText: "http://127.0.0.1:8081"
            text: kcm.plasmaKeyboardSettings.voiceInputEndpoint
            onEditingFinished: kcm.plasmaKeyboardSettings.voiceInputEndpoint = text
        }

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Model:")
            visible: kcm.plasmaKeyboardSettings.voiceInputApi === 2
            placeholderText: i18nc("@info:placeholder example model name", "e.g. Systran/faster-whisper-small")
            text: kcm.plasmaKeyboardSettings.voiceInputModel
            onEditingFinished: kcm.plasmaKeyboardSettings.voiceInputModel = text
        }

        QQC2.TextField {
            Kirigami.FormData.label: i18n("Language:")
            visible: kcm.plasmaKeyboardSettings.voiceInputApi !== 0
            placeholderText: i18nc("@info:placeholder", "Detect automatically")
            text: kcm.plasmaKeyboardSettings.voiceInputLanguage
            onEditingFinished: kcm.plasmaKeyboardSettings.voiceInputLanguage = text
        }

        Kirigami.PasswordField {
            Kirigami.FormData.label: i18n("API key:")
            visible: kcm.plasmaKeyboardSettings.voiceInputApi !== 0
            placeholderText: i18nc("@info:placeholder", "Optional")
            text: kcm.plasmaKeyboardSettings.voiceInputApiKey
            onEditingFinished: kcm.plasmaKeyboardSettings.voiceInputApiKey = text
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18n("Haptics")
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Vibrate with:")
            enabled: vibrationEnabled.checked
            model: [i18n("Automatic"), i18n("Phone vibration motor (feedbackd)"), i18n("Steam Deck")]
            currentIndex: kcm.plasmaKeyboardSettings.vibrationBackend
            onActivated: (index) => kcm.plasmaKeyboardSettings.vibrationBackend = index
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: i18n("Steam Deck:")
            enabled: vibrationEnabled.checked && kcm.plasmaKeyboardSettings.vibrationBackend !== 1
            model: [i18n("Tick the trackpads"), i18n("Rumble")]
            currentIndex: kcm.plasmaKeyboardSettings.steamDeckHapticMode
            onActivated: (index) => kcm.plasmaKeyboardSettings.steamDeckHapticMode = index
        }

        QQC2.Slider {
            Kirigami.FormData.label: i18n("Strength:")
            enabled: vibrationEnabled.checked
            from: 0
            to: 100
            stepSize: 5
            value: kcm.plasmaKeyboardSettings.vibrationStrength
            onMoved: kcm.plasmaKeyboardSettings.vibrationStrength = value
        }
    }
}
