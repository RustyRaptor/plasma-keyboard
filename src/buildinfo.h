/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QString>
#include <qqmlintegration.h>

/**
 * Which build of the keyboard is running: the version, the git commit it was built from
 * and how it was installed. Makes it possible to tell this build from the stock keyboard
 * and from older builds of the fork.
 */
class BuildInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString summary READ summary CONSTANT)

public:
    explicit BuildInfo(QObject *parent = nullptr);

    /**
     * The version of plasma-keyboard this fork is based on.
     */
    static QString version();

    /**
     * The abbreviated git commit of the source tree, with a trailing + if the tree had
     * uncommitted changes. "unknown" when the source wasn't a git checkout.
     */
    static QString commit();

    /**
     * The day of the commit, as YYYY-MM-DD, or empty when unknown.
     */
    static QString commitDate();

    /**
     * How the keyboard was installed: "Flatpak <branch>" inside a Flatpak, otherwise empty.
     */
    static QString packaging(const QString &flatpakInfoPath = QStringLiteral("/.flatpak-info"));

    static QString describe(const QString &version, const QString &commit, const QString &commitDate, const QString &packaging);

    /**
     * Everything above in one line, such as "6.8.80 fork 40f2ac8 (2026-11-03, Flatpak deck)".
     */
    static QString summary();
};
