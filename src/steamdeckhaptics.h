/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QByteArray>
#include <QString>

/**
 * Drives the Steam Deck's trackpad haptic actuators and rumble motors.
 *
 * Talks directly to the controller's vendor HID interface through hidraw
 * feature reports, using the same report layout as the kernel's hid-steam driver.
 */
class SteamDeckHaptics
{
public:
    enum class Pad : quint8 {
        Left = 0,
        Right = 1,
        Both = 2,
    };

    struct Pulse {
        quint16 onUs = 0;
        qint8 gainDb = 0;
    };

    explicit SteamDeckHaptics(const QString &devicePath);
    ~SteamDeckHaptics();

    SteamDeckHaptics(const SteamDeckHaptics &) = delete;
    SteamDeckHaptics &operator=(const SteamDeckHaptics &) = delete;

    bool isOpen() const;

    bool pulse(Pad pad, quint16 onUs, quint16 offUs, quint16 count, qint8 gainDb);
    bool rumble(quint16 intensity, quint16 leftSpeed, quint16 rightSpeed, quint8 leftGain, quint8 rightGain);

    static QByteArray hapticPulseReport(Pad pad, quint16 onUs, quint16 offUs, quint16 count, qint8 gainDb);
    static QByteArray rumbleReport(quint16 intensity, quint16 leftSpeed, quint16 rightSpeed, quint8 leftGain, quint8 rightGain);

    /**
     * Maps a feedback duration and a 0-100 strength to trackpad pulse parameters.
     */
    static Pulse pulseForFeedback(int durationMs, int strength);

    /**
     * Returns the hidraw device node of the Steam Deck controller's vendor interface, or an empty string.
     */
    static QString findDevice(const QString &sysfsRoot = QStringLiteral("/sys/class/hidraw"), const QString &devRoot = QStringLiteral("/dev"));

private:
    bool sendFeatureReport(const QByteArray &report);

    int m_fd = -1;
};
