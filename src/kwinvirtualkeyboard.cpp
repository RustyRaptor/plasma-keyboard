/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "kwinvirtualkeyboard.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QLoggingCategory>
#include <cstdio>

Q_LOGGING_CATEGORY(lcKWinVirtualKeyboard, "plasma.keyboard.kwin")

namespace
{
QString serviceName()
{
    return QStringLiteral("org.kde.KWin");
}

QString objectPath()
{
    return QStringLiteral("/VirtualKeyboard");
}

QString interfaceName()
{
    return QStringLiteral("org.kde.kwin.VirtualKeyboard");
}

QString propertiesInterface()
{
    return QStringLiteral("org.freedesktop.DBus.Properties");
}
} // namespace

KWinVirtualKeyboard::KWinVirtualKeyboard(QObject *parent)
    : QObject(parent)
{
}

bool KWinVirtualKeyboard::activeClientSupportsTextInput() const
{
    const QVariant value = property(QStringLiteral("activeClientSupportsTextInput"));
    return !value.isValid() || value.toBool();
}

bool KWinVirtualKeyboard::panelVisible() const
{
    const QVariant value = property(QStringLiteral("visible"));
    return !value.isValid() || value.toBool();
}

void KWinVirtualKeyboard::forceActivate() const
{
    QDBusMessage message = QDBusMessage::createMethodCall(serviceName(), objectPath(), interfaceName(), QStringLiteral("forceActivate"));
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected() && !bus.send(message)) {
        qCDebug(lcKWinVirtualKeyboard) << "Could not ask KWin to show the pinned keyboard";
    }
}

QVariant KWinVirtualKeyboard::property(const QString &name) const
{
    QDBusMessage message = QDBusMessage::createMethodCall(serviceName(), objectPath(), propertiesInterface(), QStringLiteral("Get"));
    message << interfaceName() << name;

    const QDBusMessage reply = QDBusConnection::sessionBus().call(message, QDBus::Block, 200);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        fprintf(stderr, "KWINPROP %s failed type=%d %s\n", qPrintable(name), int(reply.type()), qPrintable(reply.errorMessage()));
        return {};
    }

    const QVariant value = reply.arguments().constFirst().value<QDBusVariant>().variant();
    fprintf(stderr, "KWINPROP %s = %d valid=%d\n", qPrintable(name), value.toInt(), value.isValid());
    return value;
}
