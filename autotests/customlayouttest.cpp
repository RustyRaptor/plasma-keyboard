/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "customlayout.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class CustomLayoutTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void generatesRowsOfKeys()
    {
        const auto result = CustomLayout::generate("{\"rows\": [ [{\"chars\": \"ab\"}, \"backspace\"], [{\"key\": \"shift\", \"weight\": 2}, \"space\", \"enter\"] ]}");

        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QCOMPARE(result.qml.count(u"KeyboardRow {"_s), 2);
        QVERIFY(result.qml.contains(u"Key { key: 65; text: \"a\"; alternativeKeys: \"aäåãâàá\" }"_s));
        QVERIFY(result.qml.contains(u"Key { key: 66; text: \"b\" }"_s));
        QVERIFY(result.qml.contains(u"BackspaceKey { }"_s));
        QVERIFY(result.qml.contains(u"ShiftKey { sendDirectKeyEvents: true; weight: root.defaultKeyWeight * 2 }"_s));
        QVERIFY(result.qml.contains(u"SpaceKey { }"_s));
        QVERIFY(result.qml.contains(u"EnterKey { }"_s));
    }

    void keepsKeysSquareForTheWidestRow()
    {
        // Three key widths in the first row, four in the second, over two rows
        const auto result = CustomLayout::generate("{\"rows\": [ [{\"chars\": \"ab\"}, \"backspace\"], [{\"key\": \"shift\", \"weight\": 2}, \"space\", \"enter\"] ]}");

        QVERIFY(result.qml.contains(u"maxWidthToHeightRatio: 2"_s));
    }

    void arrowsBecomeACluster()
    {
        const auto result = CustomLayout::generate("{\"rows\": [[\"arrows\"]]}");

        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        const qsizetype left = result.qml.indexOf(u"Qt.Key_Left"_s);
        const qsizetype column = result.qml.indexOf(u"KeyboardColumn {"_s);
        const qsizetype up = result.qml.indexOf(u"Qt.Key_Up"_s);
        const qsizetype down = result.qml.indexOf(u"Qt.Key_Down"_s);
        const qsizetype right = result.qml.indexOf(u"Qt.Key_Right"_s);
        QVERIFY(left >= 0 && left < column && column < up && up < down && down < right);
    }

    void customAlternativesReplaceTheDefaults()
    {
        const auto result = CustomLayout::generate("{\"rows\": [[{\"chars\": \"e\", \"alternatives\": {\"e\": \"e3\"}}]]}");

        QVERIFY(result.qml.contains(u"Key { key: 69; text: \"e\"; alternativeKeys: \"e3\" }"_s));
    }

    void escapesCharacters()
    {
        const auto result = CustomLayout::generate("{\"rows\": [[{\"chars\": \"\\\"\\\\\"}]]}");

        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
        QVERIFY(result.qml.contains(u"text: \"\\\"\""_s));
        QVERIFY(result.qml.contains(u"text: \"\\\\\""_s));
    }

    void reportsErrors_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("mention");

        QTest::newRow("not json") << QByteArray("{rows") << u"JSON"_s;
        QTest::newRow("no rows") << QByteArray("{\"keys\": []}") << u"rows"_s;
        QTest::newRow("empty row") << QByteArray("{\"rows\": [[]]}") << u"row 1"_s;
        QTest::newRow("unknown key") << QByteArray("{\"rows\": [[\"tab\", \"fn\"]]}") << u"fn"_s;
        QTest::newRow("bad weight") << QByteArray("{\"rows\": [[{\"key\": \"tab\", \"weight\": 0}]]}") << u"weight"_s;
    }

    void reportsErrors()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, mention);

        const auto result = CustomLayout::generate(json);

        QVERIFY(result.qml.isEmpty());
        QVERIFY2(result.error.contains(mention), qPrintable(result.error));
    }

    void examplesAreValid_data()
    {
        QTest::addColumn<QString>("path");
        const QDir examples(QStringLiteral(QT_TESTCASE_SOURCEDIR "/../examples/custom-layouts"));
        const QStringList files = examples.entryList({u"*.json"_s});
        QVERIFY(!files.isEmpty());
        for (const QString &file : files) {
            QTest::newRow(qPrintable(file)) << examples.filePath(file);
        }
    }

    void examplesAreValid()
    {
        QFETCH(QString, path);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));

        const auto result = CustomLayout::generate(file.readAll());

        QVERIFY2(result.error.isEmpty(), qPrintable(result.error));
    }

    void writesTheLayoutForAFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(u"layout.json"_s);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{\"rows\": [[{\"chars\": \"ab\"}]]}");
        file.close();

        CustomLayout layout;
        layout.setCacheDirectory(dir.filePath(u"cache"_s));
        layout.setFile(path);

        QVERIFY(layout.url().isLocalFile());
        QFile generated(layout.url().toLocalFile());
        QVERIFY(generated.open(QIODevice::ReadOnly));
        QVERIFY(generated.readAll().contains("text: \"a\""));
        QVERIFY(layout.errorString().isEmpty());

        // Edits apply right away, under a new URL so the old component isn't reused
        const QUrl firstUrl = layout.url();
        QSignalSpy urlSpy(&layout, &CustomLayout::urlChanged);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("{\"rows\": [[{\"chars\": \"xy\"}]]}");
        file.close();
        QVERIFY(urlSpy.wait());
        QVERIFY(layout.url() != firstUrl);

        // A broken file has no layout, and says why
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("{");
        file.close();
        QVERIFY(urlSpy.wait());
        QVERIFY(layout.url().isEmpty());
        QVERIFY(!layout.errorString().isEmpty());
    }
};

QTEST_GUILESS_MAIN(CustomLayoutTest)

#include "customlayouttest.moc"
