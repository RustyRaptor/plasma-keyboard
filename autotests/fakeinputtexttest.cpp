/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "fakeinputtext.h"

#include <QTest>

using namespace Qt::StringLiterals;

class FakeInputTextTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mapsCharacters_data()
    {
        QTest::addColumn<QChar>("character");
        QTest::addColumn<int>("key");
        QTest::addColumn<bool>("shift");

        QTest::newRow("lowercase") << QChar(u'q') << int(Qt::Key_Q) << false;
        QTest::newRow("uppercase") << QChar(u'Q') << int(Qt::Key_Q) << true;
        QTest::newRow("digit") << QChar(u'7') << int(Qt::Key_7) << false;
        QTest::newRow("shifted digit") << QChar(u'&') << int(Qt::Key_7) << true;
        QTest::newRow("space") << QChar(u' ') << int(Qt::Key_Space) << false;
        QTest::newRow("newline") << QChar(u'\n') << int(Qt::Key_Return) << false;
        QTest::newRow("tab") << QChar(u'\t') << int(Qt::Key_Tab) << false;
        QTest::newRow("minus") << QChar(u'-') << int(Qt::Key_Minus) << false;
        QTest::newRow("underscore") << QChar(u'_') << int(Qt::Key_Minus) << true;
        QTest::newRow("pipe") << QChar(u'|') << int(Qt::Key_Backslash) << true;
        QTest::newRow("tilde") << QChar(u'~') << int(Qt::Key_QuoteLeft) << true;
        QTest::newRow("double quote") << QChar(u'"') << int(Qt::Key_Apostrophe) << true;
        QTest::newRow("question mark") << QChar(u'?') << int(Qt::Key_Slash) << true;
        QTest::newRow("brace") << QChar(u'{') << int(Qt::Key_BracketLeft) << true;
        QTest::newRow("colon") << QChar(u':') << int(Qt::Key_Semicolon) << true;
        QTest::newRow("greater") << QChar(u'>') << int(Qt::Key_Period) << true;
    }

    void mapsCharacters()
    {
        QFETCH(QChar, character);
        QFETCH(int, key);
        QFETCH(bool, shift);

        const auto stroke = FakeInputText::strokeForCharacter(character);
        QVERIFY(stroke.has_value());
        QCOMPARE(stroke->key, key);
        QCOMPARE(stroke->shift, shift);
    }

    void rejectsCharactersWithoutKey()
    {
        // Only the US layout can be emulated with key presses
        QVERIFY(!FakeInputText::strokeForCharacter(u'é').has_value());
        QVERIFY(!FakeInputText::strokeForCharacter(u'€').has_value());
    }

    void splitsTextIntoStrokes()
    {
        const auto strokes = FakeInputText::strokesForText(u"Hi!"_s);

        QCOMPARE(strokes.size(), 3);
        QCOMPARE(strokes.at(0).key, int(Qt::Key_H));
        QVERIFY(strokes.at(0).shift);
        QCOMPARE(strokes.at(1).key, int(Qt::Key_I));
        QVERIFY(!strokes.at(1).shift);
        QCOMPARE(strokes.at(2).key, int(Qt::Key_1));
        QVERIFY(strokes.at(2).shift);
    }

    void skipsUnmappableCharactersInText()
    {
        const auto strokes = FakeInputText::strokesForText(u"aéb"_s);

        QCOMPARE(strokes.size(), 2);
        QCOMPARE(strokes.at(1).key, int(Qt::Key_B));
    }
};

QTEST_GUILESS_MAIN(FakeInputTextTest)

#include "fakeinputtexttest.moc"
