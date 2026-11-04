/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "portalcolorscheme.h"

#include <KColorScheme>
#include <KConfig>
#include <KConfigGroup>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusVariant>
#include <QGuiApplication>
#include <QPalette>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using Settings = PortalColorScheme::Settings;

namespace
{
QVariantMap colorSet(const QString &background, const QString &foreground)
{
    return {
        {QStringLiteral("BackgroundNormal"), background},
        {QStringLiteral("BackgroundAlternate"), background},
        {QStringLiteral("ForegroundNormal"), foreground},
    };
}

// What the portal reports for a desktop using a scheme with these window colors
Settings scheme(const QString &background, const QString &foreground)
{
    Settings settings;
    for (const auto set : {"Button", "Complementary", "Header", "Selection", "Tooltip", "View", "Window"}) {
        settings.insert(QStringLiteral("org.kde.kdeglobals.Colors:") + QLatin1String(set), colorSet(background, foreground));
    }
    settings.insert(QStringLiteral("org.kde.kdeglobals.General"), {{QStringLiteral("font"), QStringLiteral("Noto Sans,11,-1,5,50,0,0,0,0,0")}});
    return settings;
}

const Settings dark = scheme(QStringLiteral("46,52,64"), QStringLiteral("236,239,244"));
const Settings light = scheme(QStringLiteral("236,239,244"), QStringLiteral("46,52,64"));
}

// Stands in for xdg-desktop-portal's Settings interface
class FakePortal : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.Settings")

public:
    Settings settings;
    int reads = 0;

public Q_SLOTS:
    Settings ReadAll(const QStringList &namespaces)
    {
        ++reads;
        Settings matching;
        for (auto it = settings.cbegin(); it != settings.cend(); ++it) {
            for (const QString &pattern : namespaces) {
                if (pattern.endsWith(u'*') ? it.key().startsWith(pattern.chopped(1)) : it.key() == pattern) {
                    matching.insert(it.key(), it.value());
                }
            }
        }
        return matching;
    }

Q_SIGNALS:
    void SettingChanged(const QString &nameSpace, const QString &key, const QDBusVariant &value);
};

class PortalColorSchemeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testWritesColorGroups();
    void testReplacesPreviousColors();
    void testIgnoresSettingsWithoutColors();
    void testAppliesColorsEachTime();
    void testFollowsPortal();

private:
    QColor kirigamiWindowColor() const;

    QTemporaryDir m_dir;
};

void PortalColorSchemeTest::initTestCase()
{
    QVERIFY(m_dir.isValid());
    qDBusRegisterMetaType<Settings>();
}

QColor PortalColorSchemeTest::kirigamiWindowColor() const
{
    // Kirigami's desktop theme builds its colors like this, with no config given
    return KColorScheme(QPalette::Active, KColorScheme::Window).background().color();
}

void PortalColorSchemeTest::testWritesColorGroups()
{
    Settings settings = dark;
    settings.insert(QStringLiteral("org.kde.kdeglobals.ColorEffects:Inactive"), {{QStringLiteral("ColorAmount"), QStringLiteral("0.025")}});
    settings.insert(QStringLiteral("org.kde.kdeglobals.WM"), {{QStringLiteral("activeBackground"), QStringLiteral("46,52,64")}});
    settings.insert(QStringLiteral("org.kde.kdeglobals.KDE"),
                    {{QStringLiteral("contrast"), QStringLiteral("4")}, {QStringLiteral("LookAndFeelPackage"), QStringLiteral("org.kde.breezedark.desktop")}});
    settings.insert(QStringLiteral("org.kde.kdeglobals.Icons"), {{QStringLiteral("Theme"), QStringLiteral("breeze-dark")}});

    KConfig config(m_dir.filePath(QStringLiteral("written")), KConfig::SimpleConfig);
    QVERIFY(PortalColorScheme::writeColors(settings, config));

    QCOMPARE(config.group(QStringLiteral("Colors:Window")).readEntry("BackgroundNormal"), QStringLiteral("46,52,64"));
    QCOMPARE(config.group(QStringLiteral("Colors:View")).readEntry("ForegroundNormal"), QStringLiteral("236,239,244"));
    QCOMPARE(config.group(QStringLiteral("ColorEffects:Inactive")).readEntry("ColorAmount"), QStringLiteral("0.025"));
    QCOMPARE(config.group(QStringLiteral("WM")).readEntry("activeBackground"), QStringLiteral("46,52,64"));
    QCOMPARE(config.group(QStringLiteral("KDE")).readEntry("contrast"), QStringLiteral("4"));
    QVERIFY(!config.group(QStringLiteral("KDE")).hasKey("LookAndFeelPackage"));
    QVERIFY(!config.hasGroup(QStringLiteral("General")));
    QVERIFY(!config.hasGroup(QStringLiteral("Icons")));
}

void PortalColorSchemeTest::testReplacesPreviousColors()
{
    KConfig config(m_dir.filePath(QStringLiteral("replaced")), KConfig::SimpleConfig);
    config.group(QStringLiteral("Colors:Selection")).writeEntry("BackgroundNormal", QStringLiteral("1,2,3"));
    config.group(QStringLiteral("Colors:Old")).writeEntry("BackgroundNormal", QStringLiteral("1,2,3"));

    QVERIFY(PortalColorScheme::writeColors(dark, config));

    QCOMPARE(config.group(QStringLiteral("Colors:Selection")).readEntry("BackgroundNormal"), QStringLiteral("46,52,64"));
    QVERIFY(!config.hasGroup(QStringLiteral("Colors:Old")));
}

void PortalColorSchemeTest::testIgnoresSettingsWithoutColors()
{
    // Not a KDE desktop: the portal has no kdeglobals colors, so keep the platform's own
    const Settings settings{{QStringLiteral("org.kde.kdeglobals.General"), {{QStringLiteral("font"), QStringLiteral("Noto Sans,11")}}}};
    KConfig config(m_dir.filePath(QStringLiteral("ignored")), KConfig::SimpleConfig);
    QVERIFY(!PortalColorScheme::writeColors(settings, config));

    PortalColorScheme colors(m_dir.filePath(QStringLiteral("ignored-applied")));
    QVERIFY(!colors.apply(settings));
    QVERIFY(qApp->property("KDE_COLOR_SCHEME_PATH").toString().isEmpty());
}

void PortalColorSchemeTest::testAppliesColorsEachTime()
{
    PortalColorScheme colors(m_dir.filePath(QStringLiteral("cache/colors")));

    QVERIFY(colors.apply(dark));
    QCOMPARE(qApp->property("KDE_COLOR_SCHEME_PATH").toString(), m_dir.filePath(QStringLiteral("cache/colors")));
    QCOMPARE(QGuiApplication::palette().color(QPalette::Window), QColor(46, 52, 64));
    QCOMPARE(kirigamiWindowColor(), QColor(46, 52, 64));

    // Switching back has to reach Kirigami too, not only the first time
    QVERIFY(colors.apply(light));
    QCOMPARE(QGuiApplication::palette().color(QPalette::Window), QColor(236, 239, 244));
    QCOMPARE(kirigamiWindowColor(), QColor(236, 239, 244));
}

void PortalColorSchemeTest::testFollowsPortal()
{
    QDBusConnection bus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("fake-portal"));
    if (!bus.isConnected()) {
        QSKIP("No session bus; run under dbus-run-session");
    }
    const QString service = QStringLiteral("org.kde.plasmakeyboard.FakePortal%1").arg(QCoreApplication::applicationPid());
    FakePortal portal;
    portal.settings = dark;
    QVERIFY(
        bus.registerObject(QStringLiteral("/org/freedesktop/portal/desktop"), &portal, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals));
    QVERIFY(bus.registerService(service));

    PortalColorScheme colors(m_dir.filePath(QStringLiteral("followed")));
    QSignalSpy applied(&colors, &PortalColorScheme::applied);
    colors.start(QDBusConnection::sessionBus(), service);
    QVERIFY(applied.wait());
    QCOMPARE(kirigamiWindowColor(), QColor(46, 52, 64));

    // Plasma rewrites a whole scheme at once; that is read again once, after the burst
    portal.settings = light;
    const int readsBefore = portal.reads;
    for (const auto set : {"Window", "View", "Button"}) {
        Q_EMIT portal.SettingChanged(QStringLiteral("org.kde.kdeglobals.Colors:") + QLatin1String(set),
                                     QStringLiteral("BackgroundNormal"),
                                     QDBusVariant(QStringLiteral("236,239,244")));
    }
    QVERIFY(applied.wait());
    QCOMPARE(kirigamiWindowColor(), QColor(236, 239, 244));
    QCOMPARE(portal.reads, readsBefore + 1);

    // Other settings don't need the colors again
    Q_EMIT portal.SettingChanged(QStringLiteral("org.kde.kdeglobals.General"), QStringLiteral("font"), QDBusVariant(QStringLiteral("Noto Sans,12")));
    QVERIFY(!applied.wait(500));
    QCOMPARE(portal.reads, readsBefore + 1);

    bus.unregisterService(service);
    QDBusConnection::disconnectFromBus(QStringLiteral("fake-portal"));
}

QTEST_MAIN(PortalColorSchemeTest)

#include "portalcolorschemetest.moc"
