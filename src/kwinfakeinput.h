/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "emittedkeys.h"

#include <QElapsedTimer>
#include <QObject>

class VkbdWaylandInputBackend;

/**
 * Sends keys unsupported by the input method protocol through KWin's fake input protocol.
 *
 * Keys sent here effectively emulate the keyboard.
 */
class KWinFakeInput : public QObject
{
    Q_OBJECT

public:
    explicit KWinFakeInput(VkbdWaylandInputBackend *backend, QObject *parent = nullptr);

    bool shouldUseFakeInput(int key) const;
    bool supportsKey(int key) const;
    bool sendKeyPressed(int key, bool pressed);

    /**
     * Types text by emulating key presses on a US keymap.
     *
     * Used when there is no input context to commit text into.
     */
    bool sendText(const QString &text);

    /**
     * Whether a key event from the keyboard grab is one this class emulated, by Linux key code.
     */
    bool isEcho(quint32 linuxKey, bool pressed);

private:
    bool ensureInitialized();
    bool isModifier(int key) const;
    void clearPressedModifiers();
    void sendFakeKeyboardKey(int key, bool pressed);

    QObject *m_fakeInput = nullptr;
    VkbdWaylandInputBackend *m_backend = nullptr;
    EmittedKeys m_emittedKeys;
    QElapsedTimer m_clock;
};
