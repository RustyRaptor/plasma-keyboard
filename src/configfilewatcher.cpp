/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "configfilewatcher.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

using namespace std::chrono_literals;

static QByteArray contentHash(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1);
}

ConfigFileWatcher::ConfigFileWatcher(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
    , m_contentHash(contentHash(path))
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(100ms);
    connect(&m_debounce, &QTimer::timeout, this, &ConfigFileWatcher::checkFile);

    // Watching the directory catches the file being created or replaced
    const QString directory = QFileInfo(path).absolutePath();
    QDir().mkpath(directory);
    m_watcher.addPath(directory);
    if (QFileInfo::exists(path)) {
        m_watcher.addPath(path);
    }

    const auto scheduleCheck = [this] {
        m_debounce.start();
    };
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, scheduleCheck);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, scheduleCheck);
}

void ConfigFileWatcher::checkFile()
{
    // A replaced file is a new inode, which has to be watched again
    if (QFileInfo::exists(m_path) && !m_watcher.files().contains(m_path)) {
        m_watcher.addPath(m_path);
    }

    const QByteArray hash = contentHash(m_path);
    if (hash == m_contentHash) {
        return;
    }
    m_contentHash = hash;
    Q_EMIT changed();
}

#include "moc_configfilewatcher.cpp"
