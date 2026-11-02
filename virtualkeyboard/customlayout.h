/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QByteArrayView>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QUrl>
#include <qqmlintegration.h>

/**
 * A keyboard layout the user describes in a JSON file, which replaces the letters layer.
 *
 * The file lists rows of keys:
 * @code
 * {"rows": [
 *     ["escape", {"chars": "qwertyuiop"}, {"key": "backspace", "weight": 2}],
 *     [{"key": "tab", "weight": 1.5}, {"chars": "asdfghjkl'"}, {"key": "enter", "weight": 1.5}],
 *     [{"key": "shift", "weight": 2}, {"chars": "zxcvbnm,.?"}, "shift"],
 *     ["symbols", "ctrl", "alt", {"key": "space", "weight": 5}, "arrows", "language", "hide"]
 * ]}
 * @endcode
 *
 * A "chars" entry is one key per character, with accented alternatives by default or
 * the ones given in "alternatives". Named keys are listed by keyNames(). "weight" is a
 * width in normal keys.
 *
 * The layout is turned into a QML layout file in the cache, which url() points to.
 * Edits to the JSON file apply right away.
 */
class CustomLayout : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QUrl url READ url NOTIFY urlChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    explicit CustomLayout(QObject *parent = nullptr);
    ~CustomLayout() override;

    struct Result {
        QString qml;
        QString error;
    };

    /**
     * Turns a JSON layout description into the QML of a keyboard layout.
     */
    static Result generate(QByteArrayView json);

    /**
     * The names that can be used for special keys.
     */
    static QStringList keyNames();

    QString file() const;
    void setFile(const QString &file);

    /**
     * The generated layout, or an empty URL without a file or with a broken one.
     */
    QUrl url() const;

    QString errorString() const;

    /**
     * Where generated layouts are written. Defaults to the application's cache.
     */
    void setCacheDirectory(const QString &directory);

Q_SIGNALS:
    void fileChanged();
    void urlChanged();
    void errorStringChanged();

private:
    void reload();
    void setResult(const QUrl &url, const QString &error);

    QString m_file;
    QString m_cacheDirectory;
    QUrl m_url;
    QString m_errorString;
    QFileSystemWatcher m_watcher;
};
