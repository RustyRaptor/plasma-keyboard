/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "emittedkeys.h"

#include <QTest>

class EmittedKeysTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void recognizesEchoOnce()
    {
        EmittedKeys keys;
        keys.note(106, true, 1000);
        keys.note(106, false, 1001);

        // The compositor hands emitted keys back through the keyboard grab
        QVERIFY(keys.takeEcho(106, true, 1010));
        QVERIFY(keys.takeEcho(106, false, 1011));

        // A second event with the same key is a real key press
        QVERIFY(!keys.takeEcho(106, true, 1020));
    }

    void distinguishesKeysAndStates()
    {
        EmittedKeys keys;
        keys.note(106, true, 1000);

        QVERIFY(!keys.takeEcho(105, true, 1001));
        QVERIFY(!keys.takeEcho(106, false, 1001));
        QVERIFY(keys.takeEcho(106, true, 1001));
    }

    void forgetsOldKeys()
    {
        EmittedKeys keys;
        keys.note(106, true, 1000);

        QVERIFY(!keys.takeEcho(106, true, 3000));
    }

    void countsRepeatedKeys()
    {
        EmittedKeys keys;
        // e.g. a cursor drag sending several arrow presses at once
        for (int i = 0; i < 3; ++i) {
            keys.note(106, true, 1000);
            keys.note(106, false, 1000);
        }

        for (int i = 0; i < 3; ++i) {
            QVERIFY(keys.takeEcho(106, true, 1005));
            QVERIFY(keys.takeEcho(106, false, 1005));
        }
        QVERIFY(!keys.takeEcho(106, true, 1005));
    }
};

QTEST_GUILESS_MAIN(EmittedKeysTest)

#include "emittedkeystest.moc"
