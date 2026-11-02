/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QByteArray>
#include <QFileSystemWatcher>
#include <QTimer>

/**
 * Notices when a config file is changed by another process.
 *
 * KConfigWatcher relies on D-Bus signals, which don't reach a Flatpak sandbox;
 * this watches the file itself. Atomic rewrites (a new file renamed over the
 * old one) are handled, and bursts of changes are reported once.
 */
class ConfigFileWatcher : public QObject
{
    Q_OBJECT

public:
    explicit ConfigFileWatcher(const QString &path, QObject *parent = nullptr);

Q_SIGNALS:
    void changed();

private:
    void checkFile();

    QString m_path;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    QByteArray m_contentHash;
};
