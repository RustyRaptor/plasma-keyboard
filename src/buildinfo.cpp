/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "buildinfo.h"

#include <plasma_keyboard_build_id.h>
#include <plasma_keyboard_version.h>

#include <QFile>
#include <QSettings>
#include <QStringList>

BuildInfo::BuildInfo(QObject *parent)
    : QObject(parent)
{
}

QString BuildInfo::version()
{
    return QStringLiteral(PLASMA_KEYBOARD_VERSION_STRING);
}

QString BuildInfo::commit()
{
    return QStringLiteral(PLASMA_KEYBOARD_BUILD_COMMIT);
}

QString BuildInfo::commitDate()
{
    return QStringLiteral(PLASMA_KEYBOARD_BUILD_COMMIT_DATE);
}

QString BuildInfo::packaging(const QString &flatpakInfoPath)
{
    if (!QFile::exists(flatpakInfoPath)) {
        return {};
    }
    const QSettings info(flatpakInfoPath, QSettings::IniFormat);
    const QString branch = info.value(QStringLiteral("Instance/branch")).toString();
    return branch.isEmpty() ? QStringLiteral("Flatpak") : QStringLiteral("Flatpak ") + branch;
}

QString BuildInfo::describe(const QString &version, const QString &commit, const QString &commitDate, const QString &packaging)
{
    QStringList details;
    if (!commitDate.isEmpty()) {
        details << commitDate;
    }
    if (!packaging.isEmpty()) {
        details << packaging;
    }

    QString text = version + QStringLiteral(" fork ") + commit;
    if (!details.isEmpty()) {
        text += QStringLiteral(" (") + details.join(QStringLiteral(", ")) + u')';
    }
    return text;
}

QString BuildInfo::summary()
{
    return describe(version(), commit(), commitDate(), packaging());
}

#include "moc_buildinfo.cpp"
