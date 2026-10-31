// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Layer with clipboard, selection and cursor movement shortcuts.

KeyboardLayout {
    id: root

    readonly property bool selecting: inputEngine && inputEngine.pressedKeys.indexOf(Qt.Key_Shift) !== -1

    KeyboardRow {
        ShortcutKey { key: Qt.Key_A; modifiers: [Qt.Key_Control]; displayText: i18n("Select all") }
        ShortcutKey { key: Qt.Key_X; modifiers: [Qt.Key_Control]; iconName: "edit-cut-symbolic" }
        ShortcutKey { key: Qt.Key_C; modifiers: [Qt.Key_Control]; iconName: "edit-copy-symbolic" }
        ShortcutKey { key: Qt.Key_V; modifiers: [Qt.Key_Control]; iconName: "edit-paste-symbolic" }
        ShortcutKey { key: Qt.Key_Z; modifiers: [Qt.Key_Control]; iconName: "edit-undo-symbolic"; repeat: true }
        ShortcutKey { key: Qt.Key_Z; modifiers: [Qt.Key_Control, Qt.Key_Shift]; iconName: "edit-redo-symbolic"; repeat: true }
    }
    KeyboardRow {
        KeysymKey { key: Qt.Key_Home; displayText: i18n("Home") }
        KeysymKey { key: Qt.Key_Up; iconName: "go-up-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_End; displayText: i18n("End") }
        ShortcutKey { key: Qt.Key_Left; modifiers: root.selecting ? [Qt.Key_Control, Qt.Key_Shift] : [Qt.Key_Control]; iconName: "go-previous-skip-symbolic"; repeat: true }
        ShortcutKey { key: Qt.Key_Right; modifiers: root.selecting ? [Qt.Key_Control, Qt.Key_Shift] : [Qt.Key_Control]; iconName: "go-next-skip-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_PageUp; displayText: i18n("PgUp") }
    }
    KeyboardRow {
        KeysymKey { key: Qt.Key_Left; iconName: "go-previous-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_Down; iconName: "go-down-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_Right; iconName: "go-next-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_Shift; sticky: true; displayText: i18n("Select") }
        ShortcutKey { key: Qt.Key_Backspace; modifiers: [Qt.Key_Control]; displayText: i18n("Del word"); repeat: true }
        KeysymKey { key: Qt.Key_PageDown; displayText: i18n("PgDn") }
    }
    KeyboardRow {
        LayerKey { targetLayer: "" }
        CursorPad { weight: 300; selecting: root.selecting }
        BackspaceKey {}
        EnterKey {}
    }
}
