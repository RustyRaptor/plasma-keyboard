/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "buildinfo.h"

#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

class BuildInfoTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCommitIsAnAbbreviatedHash();
    void testCommitDateIsADay();
    void testPackagingOutsideAFlatpak();
    void testPackagingNamesTheFlatpakBranch();
    void testPackagingWithoutABranch();
    void testDescribe();
    void testDescribeWithoutCommitDate();
    void testSummaryNamesTheCommit();
};

void BuildInfoTest::testCommitIsAnAbbreviatedHash()
{
    // A trailing + means the tree had uncommitted changes when it was built
    static const QRegularExpression hash(QStringLiteral("^[0-9a-f]{7,40}\\+?$"));
    QVERIFY2(hash.match(BuildInfo::commit()).hasMatch(), qPrintable(BuildInfo::commit()));
}

void BuildInfoTest::testCommitDateIsADay()
{
    static const QRegularExpression day(QStringLiteral("^\\d{4}-\\d{2}-\\d{2}$"));
    QVERIFY2(day.match(BuildInfo::commitDate()).hasMatch(), qPrintable(BuildInfo::commitDate()));
}

void BuildInfoTest::testPackagingOutsideAFlatpak()
{
    QTemporaryDir dir;
    QCOMPARE(BuildInfo::packaging(dir.filePath(QStringLiteral("missing"))), QString());
}

void BuildInfoTest::testPackagingNamesTheFlatpakBranch()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral(".flatpak-info"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(
        "[Application]\nname=org.kde.plasma.keyboard\nruntime=runtime/org.kde.Platform/x86_64/6.11\n\n"
        "[Instance]\ninstance-id=123\nbranch=deck\narch=x86_64\n");
    file.close();

    QCOMPARE(BuildInfo::packaging(path), QStringLiteral("Flatpak deck"));
}

void BuildInfoTest::testPackagingWithoutABranch()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral(".flatpak-info"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[Application]\nname=org.kde.plasma.keyboard\n");
    file.close();

    QCOMPARE(BuildInfo::packaging(path), QStringLiteral("Flatpak"));
}

void BuildInfoTest::testDescribe()
{
    QCOMPARE(BuildInfo::describe(QStringLiteral("6.8.80"), QStringLiteral("40f2ac8"), QStringLiteral("2026-11-03"), QStringLiteral("Flatpak deck")),
             QStringLiteral("6.8.80 fork 40f2ac8 (2026-11-03, Flatpak deck)"));
    QCOMPARE(BuildInfo::describe(QStringLiteral("6.8.80"), QStringLiteral("40f2ac8+"), QStringLiteral("2026-11-03"), QString()),
             QStringLiteral("6.8.80 fork 40f2ac8+ (2026-11-03)"));
}

void BuildInfoTest::testDescribeWithoutCommitDate()
{
    QCOMPARE(BuildInfo::describe(QStringLiteral("6.8.80"), QStringLiteral("unknown"), QString(), QString()), QStringLiteral("6.8.80 fork unknown"));
}

void BuildInfoTest::testSummaryNamesTheCommit()
{
    QVERIFY(BuildInfo::summary().contains(BuildInfo::commit()));
    QVERIFY(BuildInfo::summary().startsWith(BuildInfo::version()));
}

QTEST_GUILESS_MAIN(BuildInfoTest)

#include "buildinfotest.moc"
