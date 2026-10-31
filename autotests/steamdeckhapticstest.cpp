/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "steamdeckhaptics.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class SteamDeckHapticsTest : public QObject
{
    Q_OBJECT

private:
    void addHidrawNode(const QString &root, const QString &name, const QByteArray &hidId, const QByteArray &descriptor)
    {
        QDir().mkpath(root + u"/"_s + name + u"/device"_s);

        QFile uevent(root + u"/"_s + name + u"/device/uevent"_s);
        QVERIFY(uevent.open(QIODevice::WriteOnly));
        uevent.write("DRIVER=hid-steam\nHID_ID=" + hidId + "\nHID_NAME=Valve Software Steam Controller\n");

        QFile reportDescriptor(root + u"/"_s + name + u"/device/report_descriptor"_s);
        QVERIFY(reportDescriptor.open(QIODevice::WriteOnly));
        reportDescriptor.write(descriptor);
    }

private Q_SLOTS:
    // Report encoding

    void encodesHapticPulseReport()
    {
        const QByteArray report = SteamDeckHaptics::hapticPulseReport(SteamDeckHaptics::Pad::Both, 3000, 1000, 2, -4);

        // Feature reports are always 64 bytes, zero padded
        QCOMPARE(report.size(), 64);
        QCOMPARE(report.left(10),
                 QByteArray::fromHex("8f0802b80be803020"
                                     "0fc"));
        QCOMPARE(report.mid(10), QByteArray(54, '\0'));
    }

    void swapsPadsInHapticPulseReport()
    {
        // The firmware expects left and right swapped for legacy reasons
        QCOMPARE(SteamDeckHaptics::hapticPulseReport(SteamDeckHaptics::Pad::Left, 1, 0, 1, 0).at(2), char(1));
        QCOMPARE(SteamDeckHaptics::hapticPulseReport(SteamDeckHaptics::Pad::Right, 1, 0, 1, 0).at(2), char(0));
    }

    void encodesRumbleReport()
    {
        const QByteArray report = SteamDeckHaptics::rumbleReport(0, 20000, 10000, 1, 2);

        QCOMPARE(report.size(), 64);
        QCOMPARE(report.left(11),
                 QByteArray::fromHex("eb090000"
                                     "00204e1027"
                                     "0102"));
        QCOMPARE(report.mid(11), QByteArray(53, '\0'));
    }

    // Device discovery

    void findsControllerInterface()
    {
        QTemporaryDir sysfs;
        QVERIFY(sysfs.isValid());

        // The keyboard and mouse emulation interfaces use generic desktop usage pages
        addHidrawNode(sysfs.path(), u"hidraw0"_s, "0003:000028DE:00001205", QByteArray::fromHex("05010902a101"));
        addHidrawNode(sysfs.path(), u"hidraw1"_s, "0003:000028DE:00001205", QByteArray::fromHex("05010906a101"));
        addHidrawNode(sysfs.path(), u"hidraw2"_s, "0003:000028DE:00001205", QByteArray::fromHex("06ffff0901a101"));
        addHidrawNode(sysfs.path(), u"hidraw3"_s, "0003:0000046D:0000C077", QByteArray::fromHex("06ffff0901a101"));

        QCOMPARE(SteamDeckHaptics::findDevice(sysfs.path(), u"/dev"_s), u"/dev/hidraw2"_s);
    }

    void ignoresOtherDevices()
    {
        QTemporaryDir sysfs;
        QVERIFY(sysfs.isValid());

        // Wrong product id (a Steam Controller dongle), vendor interface
        addHidrawNode(sysfs.path(), u"hidraw0"_s, "0003:000028DE:00001142", QByteArray::fromHex("06ffff0901a101"));

        QVERIFY(SteamDeckHaptics::findDevice(sysfs.path(), u"/dev"_s).isEmpty());
        QVERIFY(SteamDeckHaptics::findDevice(sysfs.path() + u"/missing"_s, u"/dev"_s).isEmpty());
    }

    // Writing

    void unopenedDeviceRejectsWrites()
    {
        SteamDeckHaptics haptics(u"/nonexistent/hidraw"_s);

        QVERIFY(!haptics.isOpen());
        QVERIFY(!haptics.pulse(SteamDeckHaptics::Pad::Both, 1000, 1000, 1, 0));
    }

    void computesPulseForStrength_data()
    {
        QTest::addColumn<int>("durationMs");
        QTest::addColumn<int>("strength");
        QTest::addColumn<int>("expectedOnUs");
        QTest::addColumn<int>("expectedGain");

        // A 100% strength tick uses the full duration with no attenuation
        QTest::newRow("full") << 10 << 100 << 10000 << 0;
        // Lower strength attenuates the actuator gain
        QTest::newRow("half") << 10 << 50 << 10000 << -12;
        // Durations are clamped to what fits in the 16 bit field
        QTest::newRow("long") << 100 << 100 << 65535 << 0;
        // At least a one millisecond pulse, and the attenuation floor
        QTest::newRow("tiny") << 0 << 0 << 1000 << -24;
    }

    void computesPulseForStrength()
    {
        QFETCH(int, durationMs);
        QFETCH(int, strength);
        QFETCH(int, expectedOnUs);
        QFETCH(int, expectedGain);

        const auto pulse = SteamDeckHaptics::pulseForFeedback(durationMs, strength);
        QCOMPARE(int(pulse.onUs), expectedOnUs);
        QCOMPARE(int(pulse.gainDb), expectedGain);
    }
};

QTEST_GUILESS_MAIN(SteamDeckHapticsTest)

#include "steamdeckhapticstest.moc"
