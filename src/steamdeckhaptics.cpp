/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "steamdeckhaptics.h"

#include <algorithm>

#include <QDir>
#include <QFile>

#include <fcntl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

namespace
{
constexpr int FEATURE_REPORT_SIZE = 64;
constexpr char ID_TRIGGER_HAPTIC_PULSE = char(0x8f);
constexpr char ID_TRIGGER_RUMBLE_CMD = char(0xeb);

// Valve's vendor id and the Steam Deck's controller product id
constexpr QByteArrayView STEAM_DECK_HID_ID_SUFFIX = "000028DE:00001205";
// Usage Page (Vendor Defined 0xFFFF), used only by the controller interface
constexpr QByteArrayView VENDOR_USAGE_PAGE = "\x06\xff\xff";

constexpr int MIN_GAIN_DB = -24;

void appendLittleEndian(QByteArray &report, quint16 value)
{
    report.append(char(value & 0xff));
    report.append(char(value >> 8));
}

QByteArray padded(QByteArray report)
{
    report.resize(FEATURE_REPORT_SIZE, '\0');
    return report;
}
}

SteamDeckHaptics::SteamDeckHaptics(const QString &devicePath)
{
    if (!devicePath.isEmpty()) {
        m_fd = ::open(QFile::encodeName(devicePath).constData(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
    }
}

SteamDeckHaptics::~SteamDeckHaptics()
{
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

bool SteamDeckHaptics::isOpen() const
{
    return m_fd >= 0;
}

bool SteamDeckHaptics::pulse(Pad pad, quint16 onUs, quint16 offUs, quint16 count, qint8 gainDb)
{
    return sendFeatureReport(hapticPulseReport(pad, onUs, offUs, count, gainDb));
}

bool SteamDeckHaptics::rumble(quint16 intensity, quint16 leftSpeed, quint16 rightSpeed, quint8 leftGain, quint8 rightGain)
{
    return sendFeatureReport(rumbleReport(intensity, leftSpeed, rightSpeed, leftGain, rightGain));
}

QByteArray SteamDeckHaptics::hapticPulseReport(Pad pad, quint16 onUs, quint16 offUs, quint16 count, qint8 gainDb)
{
    QByteArray report;
    report.append(ID_TRIGGER_HAPTIC_PULSE);
    report.append(char(8));
    // Left and right are swapped on this report for legacy reasons
    report.append(char(pad == Pad::Both ? quint8(pad) : quint8(pad) ^ 1));
    appendLittleEndian(report, onUs);
    appendLittleEndian(report, offUs);
    appendLittleEndian(report, count);
    report.append(char(gainDb));
    return padded(report);
}

QByteArray SteamDeckHaptics::rumbleReport(quint16 intensity, quint16 leftSpeed, quint16 rightSpeed, quint8 leftGain, quint8 rightGain)
{
    QByteArray report;
    report.append(ID_TRIGGER_RUMBLE_CMD);
    report.append(char(9));
    report.append('\0');
    appendLittleEndian(report, intensity);
    appendLittleEndian(report, leftSpeed);
    appendLittleEndian(report, rightSpeed);
    report.append(char(leftGain));
    report.append(char(rightGain));
    return padded(report);
}

SteamDeckHaptics::Pulse SteamDeckHaptics::pulseForFeedback(int durationMs, int strength)
{
    const int onUs = std::clamp(durationMs * 1000, 1000, 0xffff);
    const int clampedStrength = std::clamp(strength, 0, 100);
    const int gain = qRound(MIN_GAIN_DB * (100 - clampedStrength) / 100.0);
    return {quint16(onUs), qint8(gain)};
}

QString SteamDeckHaptics::findDevice(const QString &sysfsRoot, const QString &devRoot)
{
    const QDir root(sysfsRoot);
    const QStringList nodes = root.entryList({u"hidraw*"_s}, QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &node : nodes) {
        QFile uevent(root.filePath(node + u"/device/uevent"_s));
        if (!uevent.open(QIODevice::ReadOnly)) {
            continue;
        }

        bool isSteamDeck = false;
        const QList<QByteArray> lines = uevent.readAll().split('\n');
        for (const QByteArray &line : lines) {
            if (line.startsWith("HID_ID=") && line.toUpper().endsWith(STEAM_DECK_HID_ID_SUFFIX)) {
                isSteamDeck = true;
                break;
            }
        }
        if (!isSteamDeck) {
            continue;
        }

        QFile reportDescriptor(root.filePath(node + u"/device/report_descriptor"_s));
        if (reportDescriptor.open(QIODevice::ReadOnly) && reportDescriptor.read(VENDOR_USAGE_PAGE.size()) == VENDOR_USAGE_PAGE) {
            return devRoot + u'/' + node;
        }
    }
    return {};
}

bool SteamDeckHaptics::sendFeatureReport(const QByteArray &report)
{
    if (m_fd < 0) {
        return false;
    }

    // hidraw expects the report id first; the controller uses report id 0
    QByteArray buffer(1, '\0');
    buffer.append(report);
    return ::ioctl(m_fd, HIDIOCSFEATURE(buffer.size()), buffer.data()) >= 0;
}
