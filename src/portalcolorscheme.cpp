/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "portalcolorscheme.h"

#include <KColorScheme>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>

using namespace Qt::Literals::StringLiterals;

namespace
{
constexpr auto portalPath = "/org/freedesktop/portal/desktop"_L1;
constexpr auto settingsInterface = "org.freedesktop.portal.Settings"_L1;
constexpr auto kdeglobals = "org.kde.kdeglobals."_L1;

// The parts of kdeglobals a color scheme sets, and that KColorScheme reads
bool isColorGroup(QStringView group)
{
    return group.startsWith("Colors:"_L1) || group.startsWith("ColorEffects:"_L1) || group == "WM"_L1;
}

bool isColorKey(QStringView group, QStringView key)
{
    return group == "KDE"_L1 && (key == "contrast"_L1 || key == "frameContrast"_L1);
}
}

PortalColorScheme::PortalColorScheme(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
    , m_bus(QDBusConnection::sessionBus())
{
    qDBusRegisterMetaType<Settings>();

    // Applying a scheme changes dozens of settings, one signal each
    m_rereadTimer.setSingleShot(true);
    m_rereadTimer.setInterval(100);
    connect(&m_rereadTimer, &QTimer::timeout, this, &PortalColorScheme::read);
}

bool PortalColorScheme::writeColors(const Settings &settings, KConfig &config)
{
    if (!settings.contains(kdeglobals + "Colors:View"_L1)) {
        return false;
    }

    const QStringList oldGroups = config.groupList();
    for (const QString &group : oldGroups) {
        config.deleteGroup(group);
    }

    for (auto it = settings.cbegin(); it != settings.cend(); ++it) {
        if (!it.key().startsWith(kdeglobals)) {
            continue;
        }
        const QString groupName = it.key().mid(kdeglobals.size());
        KConfigGroup group = config.group(groupName);
        for (auto entry = it.value().cbegin(); entry != it.value().cend(); ++entry) {
            if (isColorGroup(groupName) || isColorKey(groupName, entry.key())) {
                group.writeEntry(entry.key(), entry.value().toString());
            }
        }
    }
    return true;
}

bool PortalColorScheme::apply(const Settings &settings)
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    {
        KConfig file(m_path, KConfig::SimpleConfig);
        if (!writeColors(settings, file)) {
            return false;
        }
        file.sync();
    }

    // KColorScheme reads the file named here when it isn't given a config. It keeps that config open
    // and the name stays the same, so have it read the file again.
    KSharedConfig::Ptr config = KSharedConfig::openConfig(m_path);
    config->reparseConfiguration();
    qApp->setProperty("KDE_COLOR_SCHEME_PATH", m_path);

    QGuiApplication::setPalette(KColorScheme::createApplicationPalette(config));
    // Kirigami reloads its colors on this event, which setPalette() skips when the palette is the same
    QEvent paletteChange(QEvent::ApplicationPaletteChange);
    QCoreApplication::sendEvent(qApp, &paletteChange);

    Q_EMIT applied();
    return true;
}

void PortalColorScheme::start(const QDBusConnection &bus, const QString &service)
{
    m_bus = bus;
    m_service = service;
    m_bus.connect(m_service, portalPath, settingsInterface, u"SettingChanged"_s, this, SLOT(settingChanged(QString, QString, QDBusVariant)));
    read();
}

void PortalColorScheme::settingChanged(const QString &nameSpace, const QString &key, const QDBusVariant &value)
{
    Q_UNUSED(value)
    // xdg-desktop-portal-kde reports changes to kdeglobals' colors only as these
    if (nameSpace == "org.freedesktop.appearance"_L1) {
        m_rereadTimer.start();
        return;
    }
    if (!nameSpace.startsWith(kdeglobals)) {
        return;
    }
    const QStringView group = QStringView(nameSpace).mid(kdeglobals.size());
    if (isColorGroup(group) || isColorKey(group, key)) {
        m_rereadTimer.start();
    }
}

void PortalColorScheme::read()
{
    QDBusMessage message = QDBusMessage::createMethodCall(m_service, portalPath, settingsInterface, u"ReadAll"_s);
    message << QStringList{kdeglobals + u'*'};

    auto *watcher = new QDBusPendingCallWatcher(m_bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<Settings> reply = *watcher;
        if (reply.isValid()) {
            apply(reply.value());
        }
    });
}
