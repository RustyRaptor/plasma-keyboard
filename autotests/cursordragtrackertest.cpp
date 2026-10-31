/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "cursordragtracker.h"

#include <QSignalSpy>
#include <QTest>

class CursorDragTrackerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void staysInactiveForSmallMovements()
    {
        CursorDragTracker tracker;
        tracker.setActivationDistance(20);
        tracker.setStepSize(10);

        tracker.begin(QPointF(100, 50));
        // Wobbling while tapping must not move the cursor
        QCOMPARE(tracker.update(QPointF(115, 55)), QPoint(0, 0));

        QVERIFY(!tracker.active());
    }

    void activatesAndStepsFromActivationPoint()
    {
        CursorDragTracker tracker;
        tracker.setActivationDistance(20);
        tracker.setStepSize(10);
        QSignalSpy activeSpy(&tracker, &CursorDragTracker::activeChanged);

        tracker.begin(QPointF(100, 50));
        // Crossing the activation distance does not jump the cursor
        QCOMPARE(tracker.update(QPointF(125, 50)), QPoint(0, 0));
        QVERIFY(tracker.active());
        QCOMPARE(activeSpy.count(), 1);

        QCOMPARE(tracker.update(QPointF(149, 50)), QPoint(2, 0));
        // The remainder carries over to the next update
        QCOMPARE(tracker.update(QPointF(155, 50)), QPoint(1, 0));
    }

    void movesBackwards()
    {
        CursorDragTracker tracker;
        tracker.setActivationDistance(10);
        tracker.setStepSize(10);

        tracker.begin(QPointF(100, 50));
        tracker.update(QPointF(80, 50));

        QCOMPARE(tracker.update(QPointF(50, 50)), QPoint(-3, 0));
    }

    void tracksVerticalStepsWhenEnabled()
    {
        CursorDragTracker tracker;
        tracker.setActivationDistance(10);
        tracker.setStepSize(10);
        tracker.setVerticalStepSize(30);

        tracker.begin(QPointF(0, 0));
        tracker.update(QPointF(0, 15));

        QCOMPARE(tracker.update(QPointF(0, 75)), QPoint(0, 2));
    }

    void ignoresVerticalMovementByDefault()
    {
        CursorDragTracker tracker;
        tracker.setActivationDistance(10);
        tracker.setStepSize(10);

        tracker.begin(QPointF(0, 0));

        // A spacebar only moves the cursor sideways
        QCOMPARE(tracker.update(QPointF(0, 100)), QPoint(0, 0));
        QVERIFY(!tracker.active());
    }

    void endResetsState()
    {
        CursorDragTracker tracker;
        tracker.setActivationDistance(10);
        tracker.setStepSize(10);
        tracker.begin(QPointF(0, 0));
        tracker.update(QPointF(50, 0));
        QVERIFY(tracker.active());

        tracker.end();

        QVERIFY(!tracker.active());
        QCOMPARE(tracker.update(QPointF(500, 0)), QPoint(0, 0));
    }
};

QTEST_GUILESS_MAIN(CursorDragTrackerTest)

#include "cursordragtrackertest.moc"
