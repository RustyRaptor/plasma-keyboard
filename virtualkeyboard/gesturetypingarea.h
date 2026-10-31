/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QQuickItem>

class VirtualKeyboardContext;

/**
 * Lets the user type words by swiping across the letter keys inside it.
 *
 * Watches the pointer events of its child keys. Once a press that started on a
 * letter key moves far enough, the area takes over the touch point (so the key
 * doesn't type), records the path and commits the decoded word on release.
 */
class GestureTypingArea : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(VirtualKeyboardContext *virtualKeyboardContext READ virtualKeyboardContext WRITE setVirtualKeyboardContext NOTIFY virtualKeyboardContextChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

    /**
     * Whether swipes type words. Unlike enabled, this leaves the keys inside working.
     */
    Q_PROPERTY(bool swipeEnabled READ swipeEnabled WRITE setSwipeEnabled NOTIFY swipeEnabledChanged)

    /**
     * The swiped path so far, in item coordinates, for drawing a trail.
     */
    Q_PROPERTY(QList<QPointF> trail READ trail NOTIFY trailChanged)

public:
    explicit GestureTypingArea(QQuickItem *parent = nullptr);

    VirtualKeyboardContext *virtualKeyboardContext() const;
    void setVirtualKeyboardContext(VirtualKeyboardContext *context);

    bool active() const;
    bool swipeEnabled() const;
    void setSwipeEnabled(bool enabled);
    QList<QPointF> trail() const;

Q_SIGNALS:
    void virtualKeyboardContextChanged();
    void activeChanged();
    void swipeEnabledChanged();
    void trailChanged();

    /**
     * A word was typed by swiping.
     */
    void wordSwiped(const QString &word);

protected:
    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;
    void touchEvent(QTouchEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void touchUngrabEvent() override;
    void mouseUngrabEvent() override;

private:
    struct LetterKey {
        QChar letter;
        QRectF rect;
    };

    bool handlePointerEvent(QPointerEvent *event);
    QList<LetterKey> letterKeys() const;
    void begin(const QPointF &position);
    void finish();
    void reset();
    void setActive(bool active);

    QPointer<VirtualKeyboardContext> m_context;
    QList<QPointF> m_path;
    QElapsedTimer m_pressTimer;
    bool m_tracking = false;
    bool m_active = false;
    bool m_swipeEnabled = true;
};
