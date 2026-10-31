/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gesturetypingarea.h"

#include "gesturedecoder.h"
#include "inputengine.h"
#include "virtualkeyboardcontext.h"
#include "worddictionary.h"

#include <QLineF>

#include <algorithm>

namespace
{
// Distance, in key widths, a press has to travel to become a swipe
constexpr qreal ACTIVATION_DISTANCE = 0.6;
// Presses held longer than this before moving are for alternative characters, not swipes
constexpr qint64 MAXIMUM_ACTIVATION_DELAY_MS = 450;
constexpr int CANDIDATE_COUNT = 4;
}

GestureTypingArea::GestureTypingArea(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFiltersChildMouseEvents(true);
    setAcceptTouchEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

VirtualKeyboardContext *GestureTypingArea::virtualKeyboardContext() const
{
    return m_context;
}

void GestureTypingArea::setVirtualKeyboardContext(VirtualKeyboardContext *context)
{
    if (m_context == context) {
        return;
    }

    m_context = context;
    Q_EMIT virtualKeyboardContextChanged();
}

bool GestureTypingArea::active() const
{
    return m_active;
}

bool GestureTypingArea::swipeEnabled() const
{
    return m_swipeEnabled;
}

void GestureTypingArea::setSwipeEnabled(bool enabled)
{
    if (m_swipeEnabled == enabled) {
        return;
    }

    m_swipeEnabled = enabled;
    if (!m_swipeEnabled && !m_active) {
        reset();
    }
    Q_EMIT swipeEnabledChanged();
}

QList<QPointF> GestureTypingArea::trail() const
{
    return m_active ? m_path : QList<QPointF>();
}

bool GestureTypingArea::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    Q_UNUSED(item)
    if ((!m_swipeEnabled && !m_active) || !event->isPointerEvent()) {
        return false;
    }
    return handlePointerEvent(static_cast<QPointerEvent *>(event));
}

void GestureTypingArea::touchEvent(QTouchEvent *event)
{
    if (!handlePointerEvent(event)) {
        event->ignore();
    }
}

void GestureTypingArea::mouseMoveEvent(QMouseEvent *event)
{
    if (!handlePointerEvent(event)) {
        event->ignore();
    }
}

void GestureTypingArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (!handlePointerEvent(event)) {
        event->ignore();
    }
}

void GestureTypingArea::touchUngrabEvent()
{
    reset();
}

void GestureTypingArea::mouseUngrabEvent()
{
    reset();
}

bool GestureTypingArea::handlePointerEvent(QPointerEvent *event)
{
    // Swipes are single finger gestures
    if (event->pointCount() != 1) {
        if (!m_active) {
            m_tracking = false;
        }
        return m_active;
    }

    const QEventPoint &point = event->point(0);
    const QPointF position = mapFromScene(point.scenePosition());

    switch (point.state()) {
    case QEventPoint::Pressed:
        begin(position);
        return false;
    case QEventPoint::Updated:
    case QEventPoint::Stationary: {
        if (!m_tracking) {
            return false;
        }
        if (m_path.isEmpty() || m_path.constLast() != position) {
            m_path.append(position);
        }
        if (m_active) {
            Q_EMIT trailChanged();
            return true;
        }

        const QList<LetterKey> keys = letterKeys();
        if (keys.isEmpty()) {
            m_tracking = false;
            return false;
        }
        const qreal keyWidth = keys.constFirst().rect.width();
        if (QLineF(m_path.constFirst(), position).length() < ACTIVATION_DISTANCE * keyWidth) {
            return false;
        }
        if (m_pressTimer.elapsed() > MAXIMUM_ACTIVATION_DELAY_MS) {
            m_tracking = false;
            return false;
        }

        // Take the point away from the key, which cancels its press
        event->setExclusiveGrabber(point, this);
        setActive(true);
        Q_EMIT trailChanged();
        return true;
    }
    case QEventPoint::Released:
        if (m_active) {
            m_path.append(position);
            finish();
            return true;
        }
        m_tracking = false;
        return false;
    default:
        return false;
    }
}

QList<GestureTypingArea::LetterKey> GestureTypingArea::letterKeys() const
{
    QList<LetterKey> keys;
    QList<QQuickItem *> pending = childItems();
    while (!pending.isEmpty()) {
        QQuickItem *item = pending.takeLast();
        if (!item->isVisible()) {
            continue;
        }
        pending.append(item->childItems());

        // Keys sending a single letter, see Key.qml
        const QVariant text = item->property("text");
        if (!text.isValid() || !item->property("key").isValid()) {
            continue;
        }
        const QString string = text.toString();
        if (string.size() == 1 && string.front().isLetter()) {
            keys.append({string.front().toLower(), mapRectFromItem(item, item->boundingRect())});
        }
    }
    return keys;
}

void GestureTypingArea::begin(const QPointF &position)
{
    reset();

    // Only swipes starting on a letter key type words
    const QList<LetterKey> keys = letterKeys();
    const bool onLetter = std::any_of(keys.cbegin(), keys.cend(), [&position](const LetterKey &key) {
        return key.rect.contains(position);
    });
    if (!onLetter) {
        return;
    }

    m_tracking = true;
    m_path = {position};
    m_pressTimer.start();
}

void GestureTypingArea::finish()
{
    const QList<QPointF> path = m_path;
    reset();

    if (!m_context || !m_context->wordDictionary()) {
        return;
    }

    const QList<LetterKey> keys = letterKeys();
    if (keys.isEmpty()) {
        return;
    }
    QHash<QChar, QPointF> centers;
    for (const LetterKey &key : keys) {
        centers.insert(key.letter, key.rect.center());
    }

    GestureDecoder decoder;
    decoder.setKeyCenters(centers, keys.constFirst().rect.width());
    const QStringList words = decoder.decode(path, *m_context->wordDictionary(), CANDIDATE_COUNT);
    if (words.isEmpty()) {
        return;
    }

    m_context->inputEngine()->commitWord(words.constFirst(), words);
    Q_EMIT wordSwiped(words.constFirst());
}

void GestureTypingArea::reset()
{
    m_tracking = false;
    m_path.clear();
    setActive(false);
    Q_EMIT trailChanged();
}

void GestureTypingArea::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;
    Q_EMIT activeChanged();
}

#include "moc_gesturetypingarea.cpp"
