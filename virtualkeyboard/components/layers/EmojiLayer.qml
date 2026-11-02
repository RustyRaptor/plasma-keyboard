// SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

// Layer with an emoji picker, grouped by Unicode emoji category, with the recently used emoji above.

KeyboardLayout {
    id: root

    readonly property int columns: 10

    readonly property bool hasRecent: PlasmaKeyboardSettings.recentEmojis.length > 0

    // Emoji key that moves the emoji to the front of the recent ones when used
    component EmojiKey: Key {
        id: emojiKey

        required property string emoji
        required property var variants

        text: emojiKey.emoji
        alternativeKeys: emojiKey.variants
        noModifier: true
        textPixelSize: Math.round(Math.min(emojiKey.width, emojiKey.height) * 0.5)

        onClicked: emojiModel.noteUsed(emojiKey.text)
    }

    EmojiModel {
        id: emojiModel
        recent: PlasmaKeyboardSettings.recentEmojis

        onRecentChanged: {
            if (PlasmaKeyboardSettings.recentEmojis.join("") !== recent.join("")) {
                PlasmaKeyboardSettings.recentEmojis = recent;
                PlasmaKeyboardSettings.save();
            }
        }
    }

    EmojiModel {
        id: recentModel
        recent: PlasmaKeyboardSettings.recentEmojis
        currentGroup: EmojiModel.RecentGroup
    }

    // Recently used emoji, above whichever group is shown
    KeyboardRow {
        visible: root.hasRecent

        ListView {
            id: recentList
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            model: recentModel

            delegate: EmojiKey {
                width: recentList.width / root.columns
                height: recentList.height
            }
        }
    }

    KeyboardRow {
        heightWeight: root.defaultRowHeightWeight * (root.hasRecent ? 2 : 3)

        GridView {
            id: emojiGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            cellWidth: width / root.columns
            cellHeight: Math.max(1, height / (root.hasRecent ? 2 : 3))
            model: emojiModel

            delegate: EmojiKey {
                width: emojiGrid.cellWidth
                height: emojiGrid.cellHeight
            }
        }
    }

    KeyboardRow {
        LayerKey { targetLayer: ""; weight: 150 }

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
