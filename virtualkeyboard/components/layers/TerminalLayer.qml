// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Layer with keys commonly needed in a terminal: control keys, signals, navigation and shell symbols.

KeyboardLayout {
    KeyboardRow {
        KeysymKey { key: Qt.Key_Escape; displayText: "Esc" }
        KeysymKey { key: Qt.Key_Tab; displayText: "Tab"; repeat: true }
        KeysymKey { key: Qt.Key_Control; sticky: true; displayText: "Ctrl" }
        KeysymKey { key: Qt.Key_Alt; sticky: true; displayText: "Alt" }
        Key { key: Qt.Key_Bar; text: "|"; secondaryStyle: true }
        Key { key: Qt.Key_AsciiTilde; text: "~"; secondaryStyle: true }
        Key { key: Qt.Key_Slash; text: "/"; secondaryStyle: true }
        Key { key: Qt.Key_Minus; text: "-"; alternativeKeys: "-_"; smallText: "_"; secondaryStyle: true }
        KeysymKey { key: Qt.Key_Up; iconName: "go-up-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_PageUp; displayText: "PgUp" }
    }
    KeyboardRow {
        ShortcutKey { key: Qt.Key_C; modifiers: [Qt.Key_Control]; displayText: "^C" }
        ShortcutKey { key: Qt.Key_D; modifiers: [Qt.Key_Control]; displayText: "^D" }
        ShortcutKey { key: Qt.Key_Z; modifiers: [Qt.Key_Control]; displayText: "^Z" }
        ShortcutKey { key: Qt.Key_L; modifiers: [Qt.Key_Control]; displayText: "^L" }
        ShortcutKey { key: Qt.Key_R; modifiers: [Qt.Key_Control]; displayText: "^R" }
        Key { key: Qt.Key_Ampersand; text: "&"; secondaryStyle: true }
        Key { key: Qt.Key_Asterisk; text: "*"; secondaryStyle: true }
        KeysymKey { key: Qt.Key_Left; iconName: "go-previous-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_Down; iconName: "go-down-symbolic"; repeat: true }
        KeysymKey { key: Qt.Key_Right; iconName: "go-next-symbolic"; repeat: true }
    }
    KeyboardRow {
        Key { key: Qt.Key_Dollar; text: "$"; secondaryStyle: true }
        Key { key: Qt.Key_Greater; text: ">"; alternativeKeys: "><"; smallText: "<"; secondaryStyle: true }
        Key { key: Qt.Key_QuoteDbl; text: "\""; secondaryStyle: true }
        Key { key: Qt.Key_Apostrophe; text: "'"; secondaryStyle: true }
        Key { key: Qt.Key_QuoteLeft; text: "`"; secondaryStyle: true }
        Key { key: Qt.Key_BraceLeft; text: "{"; alternativeKeys: "{}"; smallText: "}"; secondaryStyle: true }
        Key { key: Qt.Key_BracketLeft; text: "["; alternativeKeys: "[]"; smallText: "]"; secondaryStyle: true }
        Key { key: Qt.Key_Semicolon; text: ";"; alternativeKeys: ";:"; smallText: ":"; secondaryStyle: true }
        Key { key: Qt.Key_Backslash; text: "\\"; secondaryStyle: true }
        KeysymKey { key: Qt.Key_PageDown; displayText: "PgDn" }
    }
    KeyboardRow {
        LayerKey { targetLayer: ""; weight: 150 }
        ShortcutKey { key: Qt.Key_A; modifiers: [Qt.Key_Control]; displayText: "^A" }
        ShortcutKey { key: Qt.Key_E; modifiers: [Qt.Key_Control]; displayText: "^E" }
        SpaceKey { weight: 300 }
        Key { key: Qt.Key_Equal; text: "="; secondaryStyle: true }
        BackspaceKey {}
        EnterKey { weight: 150 }
    }
}
