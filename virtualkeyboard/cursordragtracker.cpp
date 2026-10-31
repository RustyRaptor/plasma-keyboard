/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "cursordragtracker.h"

CursorDragTracker::CursorDragTracker(QObject *parent)
    : QObject(parent)
{
}

bool CursorDragTracker::active() const
{
    return m_active;
}

qreal CursorDragTracker::activationDistance() const
{
    return m_activationDistance;
}

void CursorDragTracker::setActivationDistance(qreal distance)
{
    if (qFuzzyCompare(m_activationDistance, distance)) {
        return;
    }

    m_activationDistance = distance;
    Q_EMIT activationDistanceChanged();
}

qreal CursorDragTracker::stepSize() const
{
    return m_stepSize;
}

void CursorDragTracker::setStepSize(qreal stepSize)
{
    if (qFuzzyCompare(m_stepSize, stepSize)) {
        return;
    }

    m_stepSize = stepSize;
    Q_EMIT stepSizeChanged();
}

qreal CursorDragTracker::verticalStepSize() const
{
    return m_verticalStepSize;
}

void CursorDragTracker::setVerticalStepSize(qreal stepSize)
{
    if (qFuzzyCompare(m_verticalStepSize, stepSize)) {
        return;
    }

    m_verticalStepSize = stepSize;
    Q_EMIT verticalStepSizeChanged();
}

void CursorDragTracker::begin(const QPointF &position)
{
    m_tracking = true;
    m_origin = position;
    setActive(false);
}

QPoint CursorDragTracker::update(const QPointF &position)
{
    if (!m_tracking) {
        return {};
    }

    const bool trackVertical = m_verticalStepSize > 0;
    const QPointF delta = position - m_origin;

    if (!m_active) {
        const qreal distance = trackVertical ? qMax(qAbs(delta.x()), qAbs(delta.y())) : qAbs(delta.x());
        if (distance < m_activationDistance) {
            return {};
        }
        // Count steps from here so activating doesn't jump the cursor
        m_origin = position;
        setActive(true);
        return {};
    }

    const int stepsX = m_stepSize > 0 ? int(delta.x() / m_stepSize) : 0;
    const int stepsY = trackVertical ? int(delta.y() / m_verticalStepSize) : 0;
    m_origin += QPointF(stepsX * m_stepSize, stepsY * m_verticalStepSize);
    return QPoint(stepsX, stepsY);
}

void CursorDragTracker::end()
{
    m_tracking = false;
    setActive(false);
}

void CursorDragTracker::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;
    Q_EMIT activeChanged();
}

#include "moc_cursordragtracker.cpp"
