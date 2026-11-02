/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "configfilewatcher.h"

#include <QSaveFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class ConfigFileWatcherTest : public QObject
{
    Q_OBJECT

private:
    static void writeAtomically(const QString &path, const QByteArray &content)
    {
        QSaveFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(content);
        QVERIFY(file.commit());
    }

private Q_SLOTS:
    void reportsAtomicRewrites()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(u"plasmakeyboardrc"_s);
        writeAtomically(path, "[General]\n");
        ConfigFileWatcher watcher(path);
        QSignalSpy spy(&watcher, &ConfigFileWatcher::changed);

        // KConfig saves by writing a new file and renaming it over the old one
        writeAtomically(path, "[General]\npanelFloating=true\n");
        QVERIFY(spy.wait(2000));

        // The watch survives the rename
        writeAtomically(path, "[General]\npanelFloating=false\n");
        QVERIFY(spy.wait(2000));
    }

    void reportsFileCreatedLater()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(u"plasmakeyboardrc"_s);
        ConfigFileWatcher watcher(path);
        QSignalSpy spy(&watcher, &ConfigFileWatcher::changed);

        // The settings file doesn't exist until something is changed
        writeAtomically(path, "[General]\n");

        QVERIFY(spy.wait(2000));
    }

    void coalescesBursts()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(u"plasmakeyboardrc"_s);
        writeAtomically(path, "a");
        ConfigFileWatcher watcher(path);
        QSignalSpy spy(&watcher, &ConfigFileWatcher::changed);

        writeAtomically(path, "b");
        writeAtomically(path, "c");
        writeAtomically(path, "d");

        QVERIFY(spy.wait(2000));
        QTest::qWait(300);
        QCOMPARE(spy.count(), 1);
    }

    void ignoresOtherFiles()
    {
        QTemporaryDir directory;
        const QString path = directory.filePath(u"plasmakeyboardrc"_s);
        writeAtomically(path, "a");
        ConfigFileWatcher watcher(path);
        QSignalSpy spy(&watcher, &ConfigFileWatcher::changed);

        writeAtomically(directory.filePath(u"kwinrc"_s), "x");

        QVERIFY(!spy.wait(500));
    }
};

QTEST_GUILESS_MAIN(ConfigFileWatcherTest)

#include "configfilewatchertest.moc"
