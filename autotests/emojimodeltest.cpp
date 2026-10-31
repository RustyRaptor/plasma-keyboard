/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "emojimodel.h"

#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

static const QByteArray SAMPLE_DATA = R"(# emoji-test.txt
# group: Smileys & Emotion

# subgroup: face-smiling
1F600                                                  ; fully-qualified     # 😀 E1.0 grinning face
263A FE0F                                              ; fully-qualified     # ☺️ E0.6 smiling face
263A                                                   ; unqualified         # ☺ E0.6 smiling face

# group: People & Body

# subgroup: hand-fingers-open
1F44B                                                  ; fully-qualified     # 👋 E0.6 waving hand
1F44B 1F3FB                                            ; fully-qualified     # 👋🏻 E1.0 waving hand: light skin tone
1F44B 1F3FF                                            ; fully-qualified     # 👋🏿 E1.0 waving hand: dark skin tone

# group: Component

# subgroup: skin-tone
1F3FB                                                  ; component           # 🏻 E1.0 light skin tone

# group: Flags

# subgroup: flag
1F3C1                                                  ; fully-qualified     # 🏁 E0.6 chequered flag
)";

class EmojiModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesGroups()
    {
        EmojiModel model;
        model.loadData(SAMPLE_DATA);

        // The component group only holds building blocks, not emoji to type
        QCOMPARE(model.groups(), QStringList({u"Smileys & Emotion"_s, u"People & Body"_s, u"Flags"_s}));
        QCOMPARE(model.groupIcons(), QStringList({u"😀"_s, u"👋"_s, u"🏁"_s}));
    }

    void filtersByGroupAndSkipsUnqualified()
    {
        EmojiModel model;
        model.loadData(SAMPLE_DATA);

        model.setCurrentGroup(0);

        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), EmojiModel::EmojiRole).toString(), u"😀"_s);
        QCOMPARE(model.data(model.index(1), EmojiModel::EmojiRole).toString(), u"☺️"_s);
        QCOMPARE(model.data(model.index(1), EmojiModel::NameRole).toString(), u"smiling face"_s);
    }

    void foldsSkinTonesIntoVariants()
    {
        EmojiModel model;
        model.loadData(SAMPLE_DATA);

        model.setCurrentGroup(1);

        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), EmojiModel::EmojiRole).toString(), u"👋"_s);
        QCOMPARE(model.data(model.index(0), EmojiModel::VariantsRole).toStringList(), QStringList({u"👋"_s, u"👋🏻"_s, u"👋🏿"_s}));
    }

    void recentGroupListsUsedEmoji()
    {
        EmojiModel model;
        model.loadData(SAMPLE_DATA);
        QSignalSpy recentSpy(&model, &EmojiModel::recentChanged);

        model.setCurrentGroup(EmojiModel::RecentGroup);
        QCOMPARE(model.rowCount(), 0);

        model.noteUsed(u"🏁"_s);
        model.noteUsed(u"😀"_s);
        model.noteUsed(u"🏁"_s);

        // Most recent first, without duplicates
        QCOMPARE(model.recent(), QStringList({u"🏁"_s, u"😀"_s}));
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(0), EmojiModel::EmojiRole).toString(), u"🏁"_s);
        QCOMPARE(model.data(model.index(0), EmojiModel::NameRole).toString(), u"chequered flag"_s);
        QCOMPARE(recentSpy.count(), 3);
    }

    void recentListIsCapped()
    {
        EmojiModel model;
        model.setMaximumRecent(2);

        model.setRecent({u"a"_s, u"b"_s, u"c"_s});
        QCOMPARE(model.recent(), QStringList({u"a"_s, u"b"_s}));

        model.noteUsed(u"d"_s);
        QCOMPARE(model.recent(), QStringList({u"d"_s, u"a"_s}));
    }

    void removesEmojiTheFontCannotShow()
    {
        static const QByteArray data = R"(# group: Smileys & Emotion
1F600 ; fully-qualified # 😀 E1.0 grinning face
1FAE0 ; fully-qualified # 🫠 E14.0 melting face
1FAE8 ; fully-qualified # 🫨 E15.0 shaking face
# group: People & Body
1F642 200D 2194 FE0F ; fully-qualified # 🙂‍↔️ E15.1 head shaking horizontally
1F44B ; fully-qualified # 👋 E0.6 waving hand
)";
        EmojiModel model;
        model.loadData(data);

        // The font lacks an emoji from 15.0, so 15.0 and newer can't be relied upon
        model.removeUnsupported([](char32_t codePoint) {
            return codePoint != 0x1FAE8;
        });

        model.setCurrentGroup(0);
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.data(model.index(1), EmojiModel::EmojiRole).toString(), u"🫠"_s);
        model.setCurrentGroup(1);
        QCOMPARE(model.rowCount(), 1);
        QCOMPARE(model.data(model.index(0), EmojiModel::EmojiRole).toString(), u"👋"_s);
    }

    void keepsEverythingWhenFontIsComplete()
    {
        EmojiModel model;
        model.loadData(SAMPLE_DATA);

        model.removeUnsupported([](char32_t) {
            return true;
        });

        model.setCurrentGroup(0);
        QCOMPARE(model.rowCount(), 2);
    }

    void loadsBundledData()
    {
        EmojiModel model;

        // The bundled data covers every Unicode emoji group except components
        QCOMPARE(model.groups().size(), 9);
        model.setCurrentGroup(0);
        QVERIFY(model.rowCount() > 100);
    }
};

QTEST_GUILESS_MAIN(EmojiModelTest)

#include "emojimodeltest.moc"
