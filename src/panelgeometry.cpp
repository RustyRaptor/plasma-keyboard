/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "panelgeometry.h"

#include <algorithm>

PanelGeometry::PanelGeometry(QObject *parent)
    : QObject(parent)
{
}

QRectF PanelGeometry::placePanel(const QSizeF &surface, const QSizeF &panel, const QPointF &position) const
{
    const qreal freeWidth = qMax(0.0, surface.width() - panel.width());
    const qreal freeHeight = qMax(0.0, surface.height() - panel.height());
    const qreal x = std::clamp(position.x(), 0.0, 1.0) * freeWidth;
    const qreal y = std::clamp(position.y(), 0.0, 1.0) * freeHeight;
    return QRectF(QPointF(x, y), panel);
}

QPointF PanelGeometry::normalizedPosition(const QSizeF &surface, const QRectF &panel) const
{
    const qreal freeWidth = surface.width() - panel.width();
    const qreal freeHeight = surface.height() - panel.height();
    const qreal x = freeWidth > 0 ? std::clamp(panel.x() / freeWidth, 0.0, 1.0) : 0.5;
    const qreal y = freeHeight > 0 ? std::clamp(panel.y() / freeHeight, 0.0, 1.0) : 0.5;
    return QPointF(x, y);
}

QRegion PanelGeometry::inputRegion(const QRect &panel, const QSize &surface)
{
    if (panel.isEmpty()) {
        return {};
    }

    QRegion region(panel);

    // Mirror the panel's horizontal extent around the surface center, and reach down to the bottom
    const int left = qMin(panel.left(), surface.width() - panel.left() - panel.width());
    const int right = qMax(panel.left() + panel.width(), surface.width() - panel.left());
    const int bottom = surface.height() - 1;

    region += QRect(left, bottom, 1, 1);
    region += QRect(right - 1, bottom, 1, 1);
    return region;
}

#include "moc_panelgeometry.cpp"
