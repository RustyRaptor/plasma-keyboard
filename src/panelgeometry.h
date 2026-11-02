/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QRegion>
#include <QSizeF>
#include <qqmlintegration.h>

/**
 * Geometry helpers for placing the keyboard panel on its full screen surface.
 */
class PanelGeometry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit PanelGeometry(QObject *parent = nullptr);

    /**
     * Places a panel inside the surface. The position is normalized: (0, 0) is
     * the top left corner and (1, 1) the bottom right one.
     *
     * The panel stays at least topInset below the top of the surface. KWin aligns the
     * bottom of an input panel with the area above a bottom panel (taskbar), which pushes
     * the top of a full screen surface off screen by the taskbar's height.
     */
    Q_INVOKABLE QRectF placePanel(const QSizeF &surface, const QSizeF &panel, const QPointF &position, qreal topInset = 0) const;

    /**
     * The normalized position of a panel rect, the inverse of placePanel.
     */
    Q_INVOKABLE QPointF normalizedPosition(const QSizeF &surface, const QRectF &panel, qreal topInset = 0) const;

    /**
     * The input region for a panel inside the surface.
     *
     * KWin positions an input panel so that the bounding box of its input region
     * is centered at the bottom of the screen. Single pixel anchors are added so that
     * the bounding box keeps the panel at its requested place.
     */
    static QRegion inputRegion(const QRect &panel, const QSize &surface);
};
