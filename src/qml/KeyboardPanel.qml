/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.plasma.keyboard
import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

Kirigami.ShadowedRectangle {
    id: root

    property var hostWindow: null
    property var virtualKeyboardContext: null
    property var inputMethodConnection: null
    property alias inputPanel: inputPanel
    property alias languagePopup: languagePopup
    property alias alternativeKeysPopup: alternativeKeysPopup
    property alias characterPopup: characterPopup
    property alias flickPreviewPopup: flickPreviewPopup
    readonly property var keyboardController: virtualKeyboardContext ? virtualKeyboardContext.keyboardController : null

    readonly property bool isFloating: PlasmaKeyboardSettings.panelFloating
    readonly property bool isFullScreenWidth: !isFloating && PlasmaKeyboardSettings.panelFillScreenWidth

    /**
     * Width of the areas beside the keys that absorb stray taps, such as from thumbs holding the device.
     */
    readonly property int sidePadding: isFloating ? PlasmaKeyboardSettings.panelSidePadding : 0

    /**
     * Whether the move and resize handles are shown.
     */
    property bool adjusting: false

    signal showSettingsRequested()

    readonly property real __baseInputAreaHeight: Math.max(Screen.height * 0.3, 150)
    readonly property real __heightScale: {
        const offset = adjustOverlay.resizeOffset.y;
        // A docked keyboard grows upwards, a floating one downwards
        const delta = isFloating ? offset : -offset;
        return clampHeightScale(PlasmaKeyboardSettings.panelHeightScale + delta / __baseInputAreaHeight);
    }
    readonly property real __widthFraction: clampWidthFraction(PlasmaKeyboardSettings.panelWidthFraction + adjustOverlay.resizeOffset.x / Screen.width)

    readonly property size __hostSize: hostWindow ? Qt.size(hostWindow.width, hostWindow.height) : Qt.size(0, 0)
    // KWin pushes the top of the surface off screen by the height of a bottom taskbar, which the keyboard can't
    // know, so a floating keyboard keeps clear of the top. Enough for the toolbar to stay reachable over usual taskbars.
    readonly property real __topInset: Kirigami.Units.gridUnit * 3
    readonly property rect __floatingRect: PanelGeometry.placePanel(__hostSize, Qt.size(width, height), Qt.point(PlasmaKeyboardSettings.panelPositionX, PlasmaKeyboardSettings.panelPositionY), __topInset)

    function clampHeightScale(scale) {
        return Math.max(0.4, Math.min(2, scale));
    }

    function clampWidthFraction(fraction) {
        return Math.max(0.25, Math.min(1, fraction));
    }

    function saveSettings() {
        PlasmaKeyboardSettings.save();
    }

    /**
     * Keeps a floating panel where a drag by the given offset left it.
     */
    function finishMove(offset) {
        const rect = Qt.rect(__floatingRect.x + offset.x, __floatingRect.y + offset.y, width, height);
        const position = PanelGeometry.normalizedPosition(__hostSize, rect, __topInset);
        PlasmaKeyboardSettings.panelPositionX = position.x;
        PlasmaKeyboardSettings.panelPositionY = position.y;
        saveSettings();
    }

    color: BreezeConstants.keyboardBackgroundColor

    // A layer fades the keyboard as one image, instead of each key adding to its background's opacity
    opacity: PlasmaKeyboardSettings.panelTransparencyEnabled ? PlasmaKeyboardSettings.panelOpacity : 1
    layer.enabled: opacity < 1

    corners {
        bottomLeftRadius: isFloating ? Kirigami.Units.cornerRadius : 0
        bottomRightRadius: isFloating ? Kirigami.Units.cornerRadius : 0
        topLeftRadius: isFullScreenWidth ? 0 : Kirigami.Units.cornerRadius
        topRightRadius: isFullScreenWidth ? 0 : Kirigami.Units.cornerRadius
    }

    shadow {
        size: isFullScreenWidth ? 0 : 16
        color: Qt.rgba(0, 0, 0, 0.3)
    }

    x: {
        if (!hostWindow) {
            return 0;
        }
        if (isFloating) {
            return __floatingRect.x + adjustOverlay.moveOffset.x + handleDragHandler.translation.x;
        }
        return (hostWindow.width / 2) - (width / 2);
    }
    y: {
        if (!hostWindow) {
            return 0;
        }
        if (isFloating) {
            return __floatingRect.y + adjustOverlay.moveOffset.y + handleDragHandler.translation.y;
        }
        return hostWindow.height - height;
    }
    width: (inputPanel.width > 0 ? inputPanel.width : 100) + sidePadding * 2
    height: topArea.height + previewArea.height + (inputPanel.height > 0 ? inputPanel.height : 100)

    onVisibleChanged: adjusting = false

    LanguagePopup {
        id: languagePopup
        keyboardPanel: inputPanel
        keyboardController: root.keyboardController
        onShowSettings: root.showSettingsRequested()
    }

    AlternativeKeysPopup {
        id: alternativeKeysPopup
        keyboardPanel: inputPanel
    }

    CharacterPopup {
        id: characterPopup
        enabled: PlasmaKeyboardSettings.characterPopupEnabled
    }

    FlickPreviewPopup {
        id: flickPreviewPopup
    }

    PreeditBubble {
        anchors.left: inputPanel.left
        anchors.top: inputPanel.top
        anchors.topMargin: -implicitHeight - Math.round(8 * BreezeConstants.scaleHint)
        inputEngine: root.virtualKeyboardContext ? root.virtualKeyboardContext.inputEngine : null
        z: 2
    }

    // Part of the keyboard above the keys that absorbs taps meant for the keys,
    // so a slightly-too-high tap doesn't fall through to the window behind
    Item {
        id: topArea
        height: PlasmaKeyboardSettings.panelTopPadding
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Rectangle {
            anchors.centerIn: parent
            visible: parent.height >= Kirigami.Units.smallSpacing * 3
            width: Kirigami.Units.gridUnit * 2
            height: Math.round(Kirigami.Units.smallSpacing / 2) * 2
            radius: height / 2
            color: BreezeConstants.keyboardTopSeparatorColor
        }

        // Swallow taps instead of passing them to the window underneath
        TapHandler {}

        // The handle bar moves a floating keyboard
        PanelDragHandler {
            id: handleDragHandler
            enabled: root.isFloating && !root.adjusting
            onFinished: (offset) => root.finishMove(offset)
        }
    }

    Rectangle {
        id: topSeparator
        height: 1
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        opacity: 0.5
        color: BreezeConstants.keyboardTopSeparatorColor
        visible: !isFloating && isFullScreenWidth
    }

    Component {
        id: toolbarComponent

        KeyboardToolbar {
            keyboardController: root.keyboardController
            inputMethodConnection: root.inputMethodConnection
            onAdjustRequested: root.adjusting = true
            onSettingsRequested: root.showSettingsRequested()
        }
    }

    // Line being typed, above the keys
    Item {
        id: previewArea
        visible: PlasmaKeyboardSettings.typingPreviewEnabled
        height: visible ? typingPreview.implicitHeight + Kirigami.Units.smallSpacing : 0
        anchors {
            top: topArea.bottom
            left: inputPanel.left
            right: inputPanel.right
            leftMargin: inputPanel.padding
            rightMargin: inputPanel.padding
        }

        TypingPreview {
            id: typingPreview
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            height: implicitHeight
            inputEngine: root.virtualKeyboardContext ? root.virtualKeyboardContext.inputEngine : null
        }
    }

    Keyboard {
        id: inputPanel
        // Keys under the adjust overlay would otherwise still type when it is tapped
        enabled: !root.adjusting
        languagePopup: languagePopup
        virtualKeyboardContext: root.virtualKeyboardContext
        toolbar: PlasmaKeyboardSettings.toolbarEnabled ? toolbarComponent : null
        heightScale: root.__heightScale
        // The side padding comes out of the floating width, so the keyboard keeps its size
        maximumWidth: root.isFloating ? Math.max(Screen.width * root.__widthFraction - root.sidePadding * 2, 100) : Screen.width
        fillWidth: root.isFloating || PlasmaKeyboardSettings.panelFillScreenWidth

        padding: (!isFloating && isFullScreenWidth) ? Kirigami.Units.smallSpacing / 2 : Kirigami.Units.largeSpacing

        anchors {
            top: previewArea.bottom
            left: parent.left
            leftMargin: root.sidePadding
        }

        function updateKeyboardLayouts() {
            if (!root.keyboardController) {
                return;
            }

            let layoutIds = PlasmaKeyboardSettings.enabledKeyboardLayoutIds;
            if (layoutIds.length === 0 && PlasmaKeyboardSettings.enabledLocales.length > 0) {
                layoutIds = root.keyboardController.layoutIdsForLocales(PlasmaKeyboardSettings.enabledLocales);
            }
            root.keyboardController.activeLayoutIds = layoutIds;
        }

        Connections {
            target: PlasmaKeyboardSettings
            function onEnabledKeyboardLayoutIdsChanged() {
                inputPanel.updateKeyboardLayouts();
            }
            function onEnabledLocalesChanged() {
                inputPanel.updateKeyboardLayouts();
            }
        }

        Component.onCompleted: {
            inputPanel.updateKeyboardLayouts();
        }
    }

    AdjustOverlay {
        id: adjustOverlay
        anchors.fill: parent
        visible: root.adjusting
        enabled: visible
        floating: root.isFloating
        z: 3

        onMoveFinished: (offset) => root.finishMove(offset)
        onResizeFinished: (offset) => {
            const delta = root.isFloating ? offset.y : -offset.y;
            PlasmaKeyboardSettings.panelHeightScale = root.clampHeightScale(PlasmaKeyboardSettings.panelHeightScale + delta / root.__baseInputAreaHeight);
            if (root.isFloating) {
                PlasmaKeyboardSettings.panelWidthFraction = root.clampWidthFraction(PlasmaKeyboardSettings.panelWidthFraction + offset.x / Screen.width);
            }
            root.saveSettings();
        }
        onFloatingToggled: {
            PlasmaKeyboardSettings.panelFloating = !PlasmaKeyboardSettings.panelFloating;
            root.saveSettings();
        }
        onResetRequested: root.hostWindow.resetPanelGeometry()
        onDoneRequested: root.adjusting = false
    }
}
