/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Covers the keyboard while adjusting its position and size.
// Dragging moves a floating keyboard; the corner handle resizes it.

Rectangle {
    id: root

    property bool floating: false

    // Live adjustments while a drag is in progress
    readonly property point moveOffset: moveHandler.active ? moveHandler.translation : Qt.point(0, 0)
    readonly property point resizeOffset: resizeHandler.active ? resizeHandler.translation : Qt.point(0, 0)

    // DragHandler that reports how far it was dragged once released.
    // activeTranslation is already back to zero when active turns false, so the last one is kept.
    component AdjustDragHandler: DragHandler {
        id: handler

        property point translation: Qt.point(0, 0)

        signal finished(point offset)

        target: null
        onActiveTranslationChanged: {
            if (handler.active) {
                handler.translation = Qt.point(handler.activeTranslation.x, handler.activeTranslation.y);
            }
        }
        onActiveChanged: {
            if (!handler.active) {
                handler.finished(handler.translation);
                handler.translation = Qt.point(0, 0);
            }
        }
    }

    signal moveFinished(point offset)
    signal resizeFinished(point offset)
    signal floatingToggled()
    signal resetRequested()
    signal doneRequested()

    color: Qt.rgba(BreezeConstants.keyboardBackgroundColor.r, BreezeConstants.keyboardBackgroundColor.g, BreezeConstants.keyboardBackgroundColor.b, 0.85)
    radius: BreezeConstants.popupRadius
    border.width: 2
    border.color: Kirigami.Theme.highlightColor

    AdjustDragHandler {
        id: moveHandler
        enabled: root.floating
        onFinished: (offset) => root.moveFinished(offset)
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.floating ? i18n("Drag to move, use the corner to resize") : i18n("Use the corner to resize")
            color: BreezeConstants.keyTextColor
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Kirigami.Units.largeSpacing

            QQC2.Button {
                icon.name: root.floating ? "go-bottom-symbolic" : "window-restore"
                text: root.floating ? i18n("Dock") : i18n("Float")
                onClicked: root.floatingToggled()
            }

            QQC2.Button {
                icon.name: "edit-reset"
                text: i18n("Reset")
                onClicked: root.resetRequested()
            }

            QQC2.Button {
                icon.name: "dialog-ok-apply"
                text: i18n("Done")
                onClicked: root.doneRequested()
            }
        }
    }

    Rectangle {
        id: resizeGrip
        width: Kirigami.Units.gridUnit * 2.5
        height: width
        radius: BreezeConstants.buttonRadius
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Kirigami.Units.smallSpacing
        color: resizeHandler.active ? Kirigami.Theme.highlightColor : BreezeConstants.secondaryKeyBackgroundColor

        Kirigami.Icon {
            anchors.centerIn: parent
            width: parent.width * 0.6
            height: width
            source: "transform-scale"
        }

        AdjustDragHandler {
            id: resizeHandler
            onFinished: (offset) => root.resizeFinished(offset)
        }
    }
}
