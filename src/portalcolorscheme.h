/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QDBusConnection>
#include <QMap>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class KConfig;
class QDBusVariant;

/**
 * Keeps a sandboxed keyboard's colors in step with the desktop's color scheme.
 *
 * Flatpak gives the sandbox the desktop's kdeglobals by binding that one file. Plasma replaces the
 * file whenever the color scheme changes, so a running sandbox keeps seeing the old one; the
 * platform theme only reloads colors from the Settings portal when the scheme's name changes,
 * which switching between light and dark variants needn't do.
 *
 * This reads the colors from the portal, writes them to a file the app owns, and makes KColorScheme
 * (and with it Kirigami) and the application palette use that file, again on every color change.
 */
class PortalColorScheme : public QObject
{
    Q_OBJECT

public:
    /// The portal's settings: namespace, such as org.kde.kdeglobals.Colors:Window, to key and value
    using Settings = QMap<QString, QVariantMap>;

    explicit PortalColorScheme(const QString &path, QObject *parent = nullptr);

    /**
     * Replaces the contents of @p config with the color scheme groups in @p settings.
     * @return false, leaving @p config alone, if @p settings has no colors
     */
    static bool writeColors(const Settings &settings, KConfig &config);

    /**
     * Writes the colors in @p settings to this object's file and makes the application use them.
     * @return false if @p settings has no colors
     */
    bool apply(const Settings &settings);

    /// Reads the colors from the Settings portal at @p service, then follows its changes
    void start(const QDBusConnection &bus = QDBusConnection::sessionBus(), const QString &service = QStringLiteral("org.freedesktop.portal.Desktop"));

Q_SIGNALS:
    /// New colors are in use
    void applied();

private Q_SLOTS:
    void settingChanged(const QString &nameSpace, const QString &key, const QDBusVariant &value);

private:
    void read();

    QString m_path;
    QDBusConnection m_bus;
    QString m_service;
    QTimer m_rereadTimer;
};
