/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <qqmlintegration.h>

/**
 * Converts a finger dragging across a key (e.g. the spacebar) into cursor movement steps.
 *
 * The drag only activates after moving past an activation distance, so normal
 * taps are unaffected. Steps are counted from where the drag activated.
 */
class CursorDragTracker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(qreal activationDistance READ activationDistance WRITE setActivationDistance NOTIFY activationDistanceChanged)
    Q_PROPERTY(qreal stepSize READ stepSize WRITE setStepSize NOTIFY stepSizeChanged)

    /**
     * Distance per vertical step, or 0 to only track horizontal movement.
     */
    Q_PROPERTY(qreal verticalStepSize READ verticalStepSize WRITE setVerticalStepSize NOTIFY verticalStepSizeChanged)

public:
    explicit CursorDragTracker(QObject *parent = nullptr);

    bool active() const;

    qreal activationDistance() const;
    void setActivationDistance(qreal distance);
    qreal stepSize() const;
    void setStepSize(qreal stepSize);
    qreal verticalStepSize() const;
    void setVerticalStepSize(qreal stepSize);

    Q_INVOKABLE void begin(const QPointF &position);

    /**
     * Returns the cursor steps since the last update. Positive x is right, positive y is down.
     */
    Q_INVOKABLE QPoint update(const QPointF &position);
    Q_INVOKABLE void end();

Q_SIGNALS:
    void activeChanged();
    void activationDistanceChanged();
    void stepSizeChanged();
    void verticalStepSizeChanged();

private:
    void setActive(bool active);

    bool m_tracking = false;
    bool m_active = false;
    QPointF m_origin;
    qreal m_activationDistance = 20;
    qreal m_stepSize = 20;
    qreal m_verticalStepSize = 0;
};
