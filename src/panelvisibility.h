/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QTimer>

/**
 * Decides whether the keyboard panel is shown.
 *
 * The panel follows the input context, with two additions:
 * - a grace period before hiding, so text inputs that are briefly torn down
 *   and recreated (common on web pages) don't make the keyboard flicker
 * - pinning, which keeps an already shown keyboard on screen without a context
 */
class PanelVisibility : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(bool detached READ detached NOTIFY detachedChanged)
    Q_PROPERTY(bool pinned READ pinned WRITE setPinned NOTIFY pinnedChanged)

public:
    explicit PanelVisibility(QObject *parent = nullptr);

    bool visible() const;

    /**
     * Whether the panel is visible without an input context to type into.
     */
    bool detached() const;

    bool hasContext() const;
    void setHasContext(bool hasContext);

    bool pinned() const;
    void setPinned(bool pinned);

    int hideDelay() const;
    void setHideDelay(int milliseconds);

    /**
     * The user hid the keyboard explicitly.
     */
    void hide();

    /**
     * The focused text input was interacted with, undoing an explicit hide.
     */
    void notifyContextActivity();

Q_SIGNALS:
    void visibleChanged();
    void detachedChanged();
    void pinnedChanged();

private:
    void update();
    void setVisible(bool visible);
    void updateDetached();

    QTimer m_hideTimer;
    bool m_visible = false;
    bool m_hasContext = false;
    bool m_pinned = false;
    bool m_userHidden = false;
    bool m_detached = false;
};
