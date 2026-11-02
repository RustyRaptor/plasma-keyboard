/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.plasma.keyboard.virtualkeyboard.components

// Shows the line being typed around the cursor, so it can be followed without looking at the text field.

Rectangle {
    id: root

    property var inputEngine: null

    // Passwords and the like are never shown
    readonly property bool __hidden: !!inputEngine && (inputEngine.inputMethodHints & (Qt.ImhHiddenText | Qt.ImhSensitiveData)) !== 0
    readonly property string __surroundingText: inputEngine ? inputEngine.surroundingText : ""
    // Without a text field (such as while pinned), what was typed recently
    readonly property bool __usesRecentText: __surroundingText.length === 0
    readonly property string __text: __usesRecentText ? (inputEngine ? inputEngine.recentText : "") : __surroundingText
    readonly property int __cursor: __usesRecentText ? __text.length : Math.min(inputEngine.cursorPosition, __text.length)

    // Only the current line
    readonly property string textBeforeCursor: {
        if (__hidden) {
            return "";
        }
        const before = __text.slice(0, __cursor);
        return before.slice(before.lastIndexOf("\n") + 1);
    }
    readonly property string textAfterCursor: {
        if (__hidden) {
            return "";
        }
        const after = __text.slice(__cursor);
        const lineEnd = after.indexOf("\n");
        return lineEnd === -1 ? after : after.slice(0, lineEnd);
    }

    implicitHeight: Kirigami.Units.gridUnit * 2
    radius: BreezeConstants.buttonRadius
    color: BreezeConstants.secondaryKeyBackgroundColor

    Item {
        anchors.fill: parent
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.rightMargin: Kirigami.Units.largeSpacing
        clip: true

        // The cursor sits in the middle, with the text before it running out to the left
        QQC2.Label {
            anchors.right: cursorBar.left
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width / 2 - cursorBar.width
            horizontalAlignment: Text.AlignRight
            elide: Text.ElideLeft
            textFormat: Text.PlainText
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.25
            text: root.textBeforeCursor
            color: BreezeConstants.keyTextColor
        }

        Rectangle {
            id: cursorBar
            visible: root.textBeforeCursor.length > 0 || root.textAfterCursor.length > 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            width: 2
            height: parent.height * 0.6
            color: Kirigami.Theme.highlightColor
        }

        QQC2.Label {
            anchors.left: cursorBar.right
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width / 2 - cursorBar.width
            elide: Text.ElideRight
            textFormat: Text.PlainText
            font.pointSize: Kirigami.Theme.defaultFont.pointSize * 1.25
            text: root.textAfterCursor
            color: BreezeConstants.keyTextColor
        }
    }
}
