// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Layer with an emoji picker, grouped by Unicode emoji category.

KeyboardLayout {
    id: root

    readonly property int columns: 10

    EmojiModel {
        id: emojiModel
        recent: PlasmaKeyboardSettings.recentEmojis
        currentGroup: PlasmaKeyboardSettings.recentEmojis.length > 0 ? EmojiModel.RecentGroup : 0

        onRecentChanged: {
            if (PlasmaKeyboardSettings.recentEmojis.join("") !== recent.join("")) {
                PlasmaKeyboardSettings.recentEmojis = recent;
                PlasmaKeyboardSettings.save();
            }
        }
    }

    KeyboardRow {
        heightWeight: root.defaultRowHeightWeight * 3

        GridView {
            id: emojiGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: width / root.columns
            cellHeight: Math.max(1, height / 3)
            model: emojiModel

            delegate: Key {
                id: emojiKey

                required property string emoji
                required property var variants

                width: emojiGrid.cellWidth
                height: emojiGrid.cellHeight
                text: emojiKey.emoji
                alternativeKeys: emojiKey.variants
                noModifier: true
                textPixelSize: Math.round(Math.min(emojiKey.width, emojiKey.height) * 0.5)

                onClicked: emojiModel.noteUsed(emojiKey.text)
            }
        }
    }

    KeyboardRow {
        LayerKey { targetLayer: ""; weight: 150 }

        ActionKey {
            iconName: "document-open-recent-symbolic"
            functionKey: true
            secondaryStyle: true
            highlighted: emojiModel.currentGroup === EmojiModel.RecentGroup
            onClicked: emojiModel.currentGroup = EmojiModel.RecentGroup
        }

        Repeater {
            model: emojiModel.groupIcons

            ActionKey {
                id: groupKey

                required property string modelData
                required property int index

                displayText: groupKey.modelData
                noModifier: true
                secondaryStyle: true
                highlighted: emojiModel.currentGroup === groupKey.index
                textPixelSize: Math.round(Math.min(groupKey.width, groupKey.height) * 0.45)
                onClicked: {
                    emojiModel.currentGroup = groupKey.index;
                    emojiGrid.positionViewAtBeginning();
                }
            }
        }

        BackspaceKey { weight: 150 }
    }
}
