/*
    SPDX-FileCopyrightText: 2024 Aleix Pol i Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QPointer>
#include <QWindow>
#include <qqmlintegration.h>

#include "inputplugin.h"
#include "panelvisibility.h"

class OverlayController;
class KWinFakeInput;
class LlmCompletionClient;
class VirtualKeyboardContext;
class VkbdWaylandInputBackend;
class VoiceInput;

/**
 * Connects the virtual keyboard UI to the compositor's input method.
 */
class InputMethodConnection : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(InputMethodConnection)

    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(bool keyboardNavigationAvailable READ keyboardNavigationAvailable WRITE setKeyboardNavigationAvailable NOTIFY keyboardNavigationAvailableChanged)
    Q_PROPERTY(bool keyboardNavigationActive READ keyboardNavigationActive WRITE setKeyboardNavigationActive NOTIFY keyboardNavigationActiveChanged)
    Q_PROPERTY(VirtualKeyboardContext *virtualKeyboardContext READ virtualKeyboardContext CONSTANT)

    /**
     * Controller for overlay popups (diacritics, emoji, text expansion).
     *
     * Exposed to QML for connecting overlay windows.
     */
    Q_PROPERTY(OverlayController *overlayController READ overlayController CONSTANT)

    /**
     * Keep the keyboard on screen when the focused text input goes away.
     */
    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY pinnedChanged)

    /**
     * Whether the keyboard is shown without an input context, typing through key emulation.
     */
    Q_PROPERTY(bool detached READ detached NOTIFY detachedChanged)

    /**
     * Speech to text through a speech recognition server.
     */
    Q_PROPERTY(VoiceInput *voiceInput READ voiceInput CONSTANT)

public:
    explicit InputMethodConnection(QObject *parent = nullptr);

    QWindow *window() const;
    void setWindow(QWindow *window);
    bool keyboardNavigationAvailable() const;
    void setKeyboardNavigationAvailable(bool available);
    bool keyboardNavigationActive() const;
    void setKeyboardNavigationActive(bool active);
    Q_INVOKABLE void hide();
    bool pinned() const;
    void setPinned(bool pinned);
    bool detached() const;
    VoiceInput *voiceInput() const;

    VirtualKeyboardContext *virtualKeyboardContext() const;

    /**
     * Get the overlay controller.
     */
    OverlayController *overlayController() const;

Q_SIGNALS:
    void windowChanged();
    void keyboardNavigationAvailableChanged();
    void keyboardNavigationActiveChanged();
    void keyNavigationPressed(int key);
    void keyNavigationReleased(int key);
    void pinnedChanged();
    void detachedChanged();

private:
    void updateWindowVisible();
    void updateSuggestionSettings();
    void updateVoiceInputSettings();

    InputPlugin m_input;
    VkbdWaylandInputBackend *m_vkbdInputBackend = nullptr;
    VirtualKeyboardContext *m_virtualKeyboardContext = nullptr;
    KWinFakeInput *m_fakeInput = nullptr;
    OverlayController *m_overlayController = nullptr;
    LlmCompletionClient *m_llmClient = nullptr;
    VoiceInput *m_voiceInput = nullptr;
    PanelVisibility m_visibility;
    QPointer<QWindow> m_window;
    bool m_keyboardNavigationAvailable = false;
    bool m_keyboardNavigationActive = false;
};
