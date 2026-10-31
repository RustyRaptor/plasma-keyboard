/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "worddictionary.h"

#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

static const QByteArray SAMPLE_WORDS = "the 1000\nthey 500\nthere 400\nthen 300\ntheme 10\ni 900\ni'm 200\nhello 50\nhelp 80\n";

class WordDictionaryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completesByFrequency()
    {
        WordDictionary dictionary;
        dictionary.loadData(SAMPLE_WORDS);

        QCOMPARE(dictionary.completions(u"the"_s, 3), QStringList({u"the"_s, u"they"_s, u"there"_s}));
        QCOMPARE(dictionary.completions(u"hel"_s, 5), QStringList({u"help"_s, u"hello"_s}));
        QVERIFY(dictionary.completions(u"xyz"_s, 5).isEmpty());
        QVERIFY(dictionary.completions(QString(), 5).isEmpty());
    }

    void exactMatchComesFirst()
    {
        WordDictionary dictionary;
        dictionary.loadData("thermal 900\nthe 100\n");

        // Typing a complete word keeps it as the first choice
        QCOMPARE(dictionary.completions(u"the"_s, 2), QStringList({u"the"_s, u"thermal"_s}));
    }

    void followsTypedCase()
    {
        WordDictionary dictionary;
        dictionary.loadData(SAMPLE_WORDS);

        QCOMPARE(dictionary.completions(u"Th"_s, 2), QStringList({u"The"_s, u"They"_s}));
        QCOMPARE(dictionary.completions(u"THE"_s, 2), QStringList({u"THE"_s, u"THEY"_s}));
    }

    void capitalizesPronounI()
    {
        WordDictionary dictionary;
        dictionary.loadData(SAMPLE_WORDS);

        QCOMPARE(dictionary.completions(u"i"_s, 2), QStringList({u"I"_s, u"I'm"_s}));
    }

    void reportsFrequencyAndMembership()
    {
        WordDictionary dictionary;
        dictionary.loadData(SAMPLE_WORDS);

        QCOMPARE(dictionary.frequency(u"they"_s), 500);
        QCOMPARE(dictionary.frequency(u"They"_s), 500);
        QCOMPARE(dictionary.frequency(u"nope"_s), 0);
        QVERIFY(dictionary.contains(u"hello"_s));
        QVERIFY(!dictionary.contains(u"nope"_s));
        QCOMPARE(dictionary.size(), 9);
    }

    void learnsWords()
    {
        QTemporaryDir directory;
        const QString learnedFile = directory.filePath(u"learned.txt"_s);

        {
            WordDictionary dictionary;
            dictionary.loadData(SAMPLE_WORDS);
            dictionary.setLearnedWordsFile(learnedFile);

            // New words become suggestions once they're typed more than once, so typos don't
            dictionary.learn(u"thexder"_s);
            QVERIFY(dictionary.completions(u"thex"_s, 3).isEmpty());
            QVERIFY(!dictionary.contains(u"thexder"_s));
            dictionary.learn(u"thexder"_s);
            QVERIFY(dictionary.completions(u"thex"_s, 3).contains(u"thexder"_s));

            // Typing a word often enough promotes it
            for (int i = 0; i < 20; ++i) {
                dictionary.learn(u"theme"_s);
            }
            QCOMPARE(dictionary.completions(u"them"_s, 1), QStringList({u"theme"_s}));
            dictionary.saveLearnedWords();
        }

        // Learned words persist
        WordDictionary dictionary;
        dictionary.loadData(SAMPLE_WORDS);
        dictionary.setLearnedWordsFile(learnedFile);
        QVERIFY(dictionary.contains(u"thexder"_s));
        QCOMPARE(dictionary.completions(u"them"_s, 1), QStringList({u"theme"_s}));
    }

    void ignoresJunkWhenLearning()
    {
        WordDictionary dictionary;
        dictionary.loadData(SAMPLE_WORDS);

        dictionary.learn(u"a1b2"_s);
        dictionary.learn(u"x"_s);
        dictionary.learn(u"https://kde.org"_s);

        QCOMPARE(dictionary.size(), 9);
    }

    void loadsBundledEnglishWords()
    {
        WordDictionary dictionary;
        QVERIFY(dictionary.loadLanguage(u"en_US"_s));

        QVERIFY(dictionary.size() > 40000);
        QCOMPARE(dictionary.completions(u"keyb"_s, 1), QStringList({u"keyboard"_s}));
        QVERIFY(!dictionary.loadLanguage(u"xx_XX"_s));
    }
};

QTEST_GUILESS_MAIN(WordDictionaryTest)

#include "worddictionarytest.moc"
