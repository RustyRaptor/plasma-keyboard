/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include "config-plasma-keyboard.h"

#include <QObject>
#include <qqmlintegration.h>

#if PLASMA_KEYBOARD_SOUNDS_ENABLED
#include <QSoundEffect>
#endif

#if PLASMA_KEYBOARD_VIBRATION_ENABLED
#include "hapticinterface.h"
#include "steamdeckhaptics.h"
#include "vibrationevent.h"

#include <memory>
#endif

/**
 * Plays the configured sound and haptic feedback for keyboard interactions.
 */
class Feedback : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Event {
        Press,
        SelectionChange,
        SelectionCommit,
    };
    Q_ENUM(Event)

    explicit Feedback(QObject *parent = nullptr);

    Q_INVOKABLE void play(Event event = Press);

private:
#if PLASMA_KEYBOARD_SOUNDS_ENABLED
    void playSound();
    QSoundEffect m_soundEffect;
#endif

#if PLASMA_KEYBOARD_VIBRATION_ENABLED
    void playVibration(Event event);
    bool playSteamDeckVibration(int durationMs);
    OrgSigxcpuFeedbackHapticInterface *m_hapticInterface = nullptr;
    std::unique_ptr<SteamDeckHaptics> m_steamDeckHaptics;
    bool m_steamDeckProbed = false;
#endif
};
