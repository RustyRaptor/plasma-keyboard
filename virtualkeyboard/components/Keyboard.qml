/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes

import org.kde.kirigami as Kirigami

import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

Item {
    id: root

    property var languagePopup: null
    property var virtualKeyboardContext: null
    property real padding: 0
    property real leftPadding: padding
    property real rightPadding: padding
    property real topPadding: padding
    property real bottomPadding: padding

    /**
     * Multiplier for the height of the keys.
     */
    property real heightScale: 1

    /**
     * Width available to the keyboard.
     */
    property real maximumWidth: Screen.width

    /**
     * Whether the keyboard stretches to fill maximumWidth.
     */
    property bool fillWidth: PlasmaKeyboardSettings.panelFillScreenWidth

    /**
     * Toolbar shown in the candidate strip while there are no word candidates.
     */
    property Component toolbar: null

    readonly property var inputEngine: virtualKeyboardContext ? virtualKeyboardContext.inputEngine : VirtualKeyboard.inputEngine
    readonly property var keyboardController: virtualKeyboardContext ? virtualKeyboardContext.keyboardController : VirtualKeyboard.keyboardController
    readonly property var keyboardPackageResolver: virtualKeyboardContext ? virtualKeyboardContext.keyboardPackageResolver : VirtualKeyboard.keyboardPackageResolver
    readonly property bool navigationAvailable: keyboardNavigation.navigationAvailable
    property alias navigationModeActive: keyboardNavigation.navigationModeActive

    readonly property string layoutId: keyboardController ? keyboardController.layoutId : ""
    readonly property string packageId: keyboardPackageResolver ? keyboardPackageResolver.packageId(layoutId) : ""
    readonly property string layoutType: keyboardController ? keyboardController.layoutType : "main"
    readonly property var layoutDefinition: layoutDefinitionLoader.item
    readonly property var textComposer: (layoutLoader.item && layoutLoader.item.textComposer) || (layoutDefinition ? layoutDefinition.textComposer : null)

    implicitHeight: __keyboardHeight
    implicitWidth: __keyboardWidth

    // Always have the keyboard panel be 30% of the screen height, or 150px, whichever is larger
    readonly property real __baseKeyboardInputAreaHeight: Math.max(Screen.height * 0.3, 150) * heightScale
    readonly property real __keyboardInputAreaHeight: __baseKeyboardInputAreaHeight * (layoutLoader.item?.panelHeightFactor ?? 1)
    readonly property real __contentHeight: __keyboardInputAreaHeight + (candidateStrip.visible ? candidateStrip.implicitHeight : 0)
    readonly property real __horizontalPadding: leftPadding + rightPadding
    readonly property real __verticalPadding: topPadding + bottomPadding
    readonly property real __keyboardHeight: __contentHeight + __verticalPadding
    readonly property real __keyboardWidth: Math.round(__contentHeight * aspectRatio) + __horizontalPadding

    // The value to multiply the height by to get the width
    readonly property real aspectRatio: {
        // Ratio to just fill the available width
        const availableWidth = Math.max(0, maximumWidth - __horizontalPadding);
        const fillScreenWidthRatio = availableWidth / __contentHeight;
        if (fillWidth) {
            return fillScreenWidthRatio;
        }

        const targetAspectRatio = 3.0; // Target width ratio: 3 * height
        return Math.min(fillScreenWidthRatio, targetAspectRatio);
    }

    // The design ratios for the keyboard height
    readonly property real __keyboardDesignHeight: {
        if (Screen.width < 500) {
            // Phone mode
            return 800;
        }
        if (Screen.width < 1200) {
            // Wider
            return 600;
        }
        // Widest
        return 700;
    }
    readonly property real panelScaleHint: Math.max(0.3, __keyboardInputAreaHeight / __keyboardDesignHeight)

    // Layers that layouts don't need to provide themselves
    readonly property var __builtinLayers: ({
        "editing": editingLayer,
        "terminal": terminalLayer,
        "emoji": emojiLayer,
    })

    Component {
        id: editingLayer
        EditingLayer {}
    }

    Component {
        id: terminalLayer
        TerminalLayer {}
    }

    Component {
        id: emojiLayer
        EmojiLayer {}
    }

    function resetNavigation() {
        keyboardNavigation.resetNavigation();
    }

    function handleNavigationPressed(key) {
        keyboardNavigation.handleNavigationPressed(key);
    }

    function handleNavigationReleased(key) {
        keyboardNavigation.handleNavigationReleased(key);
    }

    function updateScaleHint() {
        if (width <= 0) {
            return;
        }
        BreezeConstants.scaleHint = panelScaleHint;
    }

    Binding {
        target: root.inputEngine
        property: "textComposer"
        value: root.textComposer
        when: root.inputEngine !== null && root.textComposer !== null
    }

    onPanelScaleHintChanged: updateScaleHint()
    Component.onCompleted: updateScaleHint()

    Loader {
        id: layoutDefinitionLoader
        source: root.keyboardPackageResolver ? root.keyboardPackageResolver.layoutUrl(root.layoutId) : ""

        onStatusChanged: {
            if (status === Loader.Error) {
                console.warn("Failed to load keyboard layout definition", root.layoutId, source);
            }
        }
    }

    KeyboardNavigation {
        id: keyboardNavigation
        layoutLoader: layoutLoader
        keyboardStrip: candidateStrip
        languagePopup: root.languagePopup
    }

    ColumnLayout {
        anchors {
            fill: parent
            leftMargin: root.leftPadding
            rightMargin: root.rightPadding
            topMargin: root.topPadding
            bottomMargin: root.bottomPadding
        }
        spacing: 0

        KeyboardStrip {
            id: candidateStrip
            Layout.fillWidth: true
            inputEngine: root.inputEngine
            toolbar: root.toolbar
        }

        GestureTypingArea {
            id: gestureArea

            readonly property int __blockingHints: Qt.ImhSensitiveData | Qt.ImhHiddenText | Qt.ImhUrlCharactersOnly | Qt.ImhEmailCharactersOnly
                | Qt.ImhDigitsOnly | Qt.ImhFormattedNumbersOnly | Qt.ImhDialableCharactersOnly

            Layout.preferredWidth: parent.width
            Layout.maximumWidth: (layoutLoader.item && layoutLoader.item.maxWidthToHeightRatio !== -1) ? (layoutLoader.item.maxWidthToHeightRatio * height) : parent.width
            Layout.preferredHeight: root.__keyboardInputAreaHeight
            Layout.alignment: Qt.AlignHCenter

            virtualKeyboardContext: root.virtualKeyboardContext
            swipeEnabled: PlasmaKeyboardSettings.gestureTypingEnabled && root.layoutType === "main"
                && !(root.inputEngine && (root.inputEngine.inputMethodHints & __blockingHints))

            Loader {
                id: layoutLoader

                anchors.fill: parent

                sourceComponent: {
                    if (!root.layoutDefinition) {
                        return null;
                    }
                    return root.layoutDefinition[root.layoutType] ?? root.__builtinLayers[root.layoutType] ?? root.layoutDefinition.main;
                }

                onLoaded: {
                    resetNavigation();
                    if (item && item.virtualKeyboardContext !== undefined) {
                        item.virtualKeyboardContext = root.virtualKeyboardContext;
                    }
                    if (item && item.layoutId !== undefined) {
                        item.layoutId = root.layoutId;
                    }
                    if (item && item.packageId !== undefined) {
                        item.packageId = root.packageId;
                    }
                }

                onStatusChanged: {
                    if (status === Loader.Error) {
                        console.warn("Failed to load keyboard layout", root.layoutId, root.layoutType);
                    }
                }
            }

            // Trail following the finger while swiping
            Shape {
                anchors.fill: parent
                visible: gestureArea.active
                z: 10

                ShapePath {
                    strokeColor: Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.7)
                    strokeWidth: Math.round(16 * BreezeConstants.scaleHint)
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin

                    PathPolyline {
                        path: gestureArea.trail
                    }
                }
            }
        }
    }
}
