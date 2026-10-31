/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Icon button shown in the keyboard toolbar.

Item {
    id: root

    property string iconName
    property string toolTip
    property bool checked: false

    signal clicked()

    Layout.fillHeight: true
    Layout.preferredWidth: height * 1.4

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width, parent.height) * 0.9
        height: width
        radius: BreezeConstants.buttonRadius
        color: (root.checked || tapHandler.pressed) ? BreezeConstants.popupHighlightColor : "transparent"
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        width: Math.round(parent.height * 0.5)
        height: width
        source: root.iconName
        color: BreezeConstants.selectionListTextColor
    }

    TapHandler {
        id: tapHandler

        onPressedChanged: {
            if (pressed) {
                Feedback.play(Feedback.Press);
            }
        }
        onTapped: root.clicked()
    }
}
