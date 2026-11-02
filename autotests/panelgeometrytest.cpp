/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "panelgeometry.h"

#include <QTest>

class PanelGeometryTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    // Placement

    void placesPanelWithinSurface_data()
    {
        QTest::addColumn<QPointF>("position");
        QTest::addColumn<QRectF>("expected");

        QTest::newRow("bottom center") << QPointF(0.5, 1) << QRectF(200, 500, 400, 100);
        QTest::newRow("top left") << QPointF(0, 0) << QRectF(0, 0, 400, 100);
        QTest::newRow("bottom right") << QPointF(1, 1) << QRectF(400, 500, 400, 100);
        // Out of range positions are clamped to keep the panel on screen
        QTest::newRow("clamped") << QPointF(-1, 3) << QRectF(0, 500, 400, 100);
    }

    void placesPanelWithinSurface()
    {
        QFETCH(QPointF, position);
        QFETCH(QRectF, expected);

        PanelGeometry geometry;
        QCOMPARE(geometry.placePanel(QSizeF(800, 600), QSizeF(400, 100), position), expected);
    }

    void panelLargerThanSurfaceIsPinnedToOrigin()
    {
        PanelGeometry geometry;
        QCOMPARE(geometry.placePanel(QSizeF(300, 600), QSizeF(400, 100), QPointF(1, 1)), QRectF(0, 500, 400, 100));
    }

    void convertsDraggedPositionBack()
    {
        PanelGeometry geometry;

        // Inverse of placePanel, used to persist a dragged panel position
        QCOMPARE(geometry.normalizedPosition(QSizeF(800, 600), QRectF(100, 250, 400, 100)), QPointF(0.25, 0.5));
        QCOMPARE(geometry.normalizedPosition(QSizeF(800, 600), QRectF(-50, 900, 400, 100)), QPointF(0, 1));
        // No free space on an axis keeps it centered
        QCOMPARE(geometry.normalizedPosition(QSizeF(400, 100), QRectF(0, 0, 400, 100)), QPointF(0.5, 0.5));
    }

    void keepsPanelBelowTopInset()
    {
        PanelGeometry geometry;

        // The top of the surface can be off screen, so the topmost position starts below the inset
        QCOMPARE(geometry.placePanel(QSizeF(800, 600), QSizeF(400, 100), QPointF(0, 0), 50), QRectF(0, 50, 400, 100));
        QCOMPARE(geometry.placePanel(QSizeF(800, 600), QSizeF(400, 100), QPointF(0, 0.5), 50), QRectF(0, 275, 400, 100));
        QCOMPARE(geometry.placePanel(QSizeF(800, 600), QSizeF(400, 100), QPointF(0, 1), 50), QRectF(0, 500, 400, 100));

        QCOMPARE(geometry.normalizedPosition(QSizeF(800, 600), QRectF(0, 275, 400, 100), 50), QPointF(0, 0.5));
        // Dragged into the inset is the topmost position
        QCOMPARE(geometry.normalizedPosition(QSizeF(800, 600), QRectF(0, 10, 400, 100), 50), QPointF(0, 0));
    }

    // Input region

    void dockedPanelRegionIsThePanel()
    {
        const QRect panel(200, 500, 400, 100);

        const QRegion region = PanelGeometry::inputRegion(panel, QSize(800, 600));

        QCOMPARE(region, QRegion(panel));
    }

    void floatingPanelRegionAnchorsToBottomCenter()
    {
        // The compositor centers the input region's bounding box at the bottom of the
        // screen, so it must be symmetric around the center and reach the bottom edge
        const QRect panel(0, 100, 300, 100);

        const QRegion region = PanelGeometry::inputRegion(panel, QSize(800, 600));

        QCOMPARE(region.boundingRect(), QRect(0, 100, 800, 500));
        QVERIFY(region.contains(panel));
        // Only single pixel anchors are added outside of the panel
        QCOMPARE((region - QRegion(panel)).rectCount(), 2);
        QCOMPARE((region - QRegion(panel)).boundingRect().height(), 1);
    }

    void centeredFloatingPanelOnlyNeedsBottomAnchor()
    {
        const QRect panel(200, 100, 400, 100);

        const QRegion region = PanelGeometry::inputRegion(panel, QSize(800, 600));

        QCOMPARE(region.boundingRect(), QRect(200, 100, 400, 500));
    }

    void emptyPanelHasNoRegion()
    {
        QVERIFY(PanelGeometry::inputRegion(QRect(), QSize(800, 600)).isEmpty());
    }
};

QTEST_GUILESS_MAIN(PanelGeometryTest)

#include "panelgeometrytest.moc"
