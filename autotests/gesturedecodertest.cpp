/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gesturedecoder.h"
#include "worddictionary.h"

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QTest>

using namespace Qt::StringLiterals;

static constexpr qreal KEY_WIDTH = 100;

// Key centers of a QWERTY layout with 100x100 keys and staggered rows
static QHash<QChar, QPointF> qwertyKeyCenters()
{
    const QStringList rows = {u"qwertyuiop"_s, u"asdfghjkl"_s, u"zxcvbnm"_s};
    const QList<qreal> offsets = {50, 100, 200};
    QHash<QChar, QPointF> centers;
    for (int row = 0; row < rows.size(); ++row) {
        for (int column = 0; column < rows.at(row).size(); ++column) {
            centers.insert(rows.at(row).at(column), QPointF(offsets.at(row) + column * KEY_WIDTH, 50 + row * KEY_WIDTH));
        }
    }
    return centers;
}

// A sloppy swipe through the letters of a word: wobbly, with corners cut short
static QList<QPointF> swipe(const QString &word, quint32 seed)
{
    const auto centers = qwertyKeyCenters();
    QRandomGenerator random(seed);
    QList<QPointF> path;
    for (int i = 0; i < word.size(); ++i) {
        const QPointF jitter((random.bounded(60) - 30), (random.bounded(60) - 30));
        const QPointF point = centers.value(word.at(i)) + jitter;
        if (!path.isEmpty()) {
            const QPointF previous = path.constLast();
            for (int step = 1; step < 8; ++step) {
                path.append(previous + (point - previous) * (step / 8.0));
            }
        }
        path.append(point);
    }
    return path;
}

class GestureDecoderTest : public QObject
{
    Q_OBJECT

private:
    WordDictionary sampleDictionary()
    {
        WordDictionary dictionary;
        dictionary.loadData(
            "the 1000\nto 900\ntoo 300\nhello 400\nhell 100\nworld 350\nword 300\nwould 500\nkeyboard 50\ntest 200\nrest 150\ntear 100\nplasma 20\n"
            "deck 40\ndesk 60\ntyping 30\ntoping 1\n");
        return dictionary;
    }

private Q_SLOTS:
    void decodesSloppySwipes_data()
    {
        QTest::addColumn<QString>("word");

        for (const QString &word : {u"hello"_s, u"world"_s, u"keyboard"_s, u"test"_s, u"plasma"_s, u"deck"_s, u"typing"_s, u"would"_s}) {
            QTest::newRow(qPrintable(word)) << word;
        }
    }

    void decodesSloppySwipes()
    {
        QFETCH(QString, word);
        const WordDictionary dictionary = sampleDictionary();
        GestureDecoder decoder;
        decoder.setKeyCenters(qwertyKeyCenters(), KEY_WIDTH);

        for (quint32 seed = 1; seed <= 5; ++seed) {
            const QStringList result = decoder.decode(swipe(word, seed), dictionary, 3);
            QVERIFY2(!result.isEmpty(), qPrintable(word));
            QCOMPARE(result.first(), word);
        }
    }

    void prefersFrequentWordsWithTheSamePath()
    {
        const WordDictionary dictionary = sampleDictionary();
        GestureDecoder decoder;
        decoder.setKeyCenters(qwertyKeyCenters(), KEY_WIDTH);

        // "to" and "too" trace the same keys
        const QStringList result = decoder.decode(swipe(u"to"_s, 1), dictionary, 3);
        QCOMPARE(result.first(), u"to"_s);
        QVERIFY(result.contains(u"too"_s));
    }

    void offersAlternatives()
    {
        const WordDictionary dictionary = sampleDictionary();
        GestureDecoder decoder;
        decoder.setKeyCenters(qwertyKeyCenters(), KEY_WIDTH);

        const QStringList result = decoder.decode(swipe(u"desk"_s, 3), dictionary, 3);
        QCOMPARE(result.first(), u"desk"_s);
        QVERIFY(result.contains(u"deck"_s));
        QVERIFY(result.size() <= 3);
    }

    void ignoresTinyPaths()
    {
        const WordDictionary dictionary = sampleDictionary();
        GestureDecoder decoder;
        decoder.setKeyCenters(qwertyKeyCenters(), KEY_WIDTH);

        QVERIFY(decoder.decode({QPointF(450, 50), QPointF(455, 52)}, dictionary, 3).isEmpty());
        QVERIFY(decoder.decode({}, dictionary, 3).isEmpty());
    }

    void needsKeyCenters()
    {
        const WordDictionary dictionary = sampleDictionary();
        GestureDecoder decoder;

        QVERIFY(decoder.decode(swipe(u"hello"_s, 1), dictionary, 3).isEmpty());
    }

    void decodesWithBundledDictionary()
    {
        WordDictionary dictionary;
        QVERIFY(dictionary.loadLanguage(u"en"_s));
        GestureDecoder decoder;
        decoder.setKeyCenters(qwertyKeyCenters(), KEY_WIDTH);

        // A clean swipe should find common words among tens of thousands
        int found = 0;
        QElapsedTimer timer;
        timer.start();
        const QStringList words = {u"hello"_s, u"world"_s, u"keyboard"_s, u"people"_s, u"because"_s, u"something"_s, u"water"_s, u"thanks"_s};
        for (const QString &word : words) {
            const QStringList result = decoder.decode(swipe(word, 7), dictionary, 3);
            if (!result.isEmpty() && result.first() == word) {
                ++found;
            } else {
                qInfo() << "Swiping" << word << "gave" << result;
            }
        }
        QVERIFY2(found >= words.size() - 1, qPrintable(QString::number(found)));
        qInfo() << "Average decoding time" << timer.elapsed() / words.size() << "ms";
    }
};

QTEST_GUILESS_MAIN(GestureDecoderTest)

#include "gesturedecodertest.moc"
