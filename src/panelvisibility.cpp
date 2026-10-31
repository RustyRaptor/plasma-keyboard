/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "panelvisibility.h"

PanelVisibility::PanelVisibility(QObject *parent)
    : QObject(parent)
{
    m_hideTimer.setSingleShot(true);
    m_hideTimer.setInterval(0);
    connect(&m_hideTimer, &QTimer::timeout, this, &PanelVisibility::update);
}

bool PanelVisibility::visible() const
{
    return m_visible;
}

bool PanelVisibility::detached() const
{
    return m_detached;
}

bool PanelVisibility::hasContext() const
{
    return m_hasContext;
}

void PanelVisibility::setHasContext(bool hasContext)
{
    if (m_hasContext == hasContext) {
        return;
    }

    m_hasContext = hasContext;
    if (m_hasContext) {
        m_hideTimer.stop();
    } else {
        // A new context starts out shown again
        m_userHidden = false;
        if (m_visible && !m_pinned && m_hideTimer.interval() > 0) {
            m_hideTimer.start();
        }
    }
    update();
}

bool PanelVisibility::pinned() const
{
    return m_pinned;
}

void PanelVisibility::setPinned(bool pinned)
{
    if (m_pinned == pinned) {
        return;
    }

    m_pinned = pinned;
    Q_EMIT pinnedChanged();
    update();
}

int PanelVisibility::hideDelay() const
{
    return m_hideTimer.interval();
}

void PanelVisibility::setHideDelay(int milliseconds)
{
    m_hideTimer.setInterval(qMax(0, milliseconds));
}

void PanelVisibility::hide()
{
    m_userHidden = true;
    m_hideTimer.stop();
    update();
}

void PanelVisibility::notifyContextActivity()
{
    if (!m_hasContext || !m_userHidden) {
        return;
    }

    m_userHidden = false;
    update();
}

void PanelVisibility::update()
{
    if (m_userHidden) {
        setVisible(false);
    } else if (m_hasContext) {
        setVisible(true);
    } else if (m_hideTimer.isActive()) {
        // Keep the current state during the grace period
    } else {
        setVisible(m_visible && m_pinned);
    }
    updateDetached();
}

void PanelVisibility::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }

    m_visible = visible;
    Q_EMIT visibleChanged();
}

void PanelVisibility::updateDetached()
{
    const bool detached = m_visible && !m_hasContext;
    if (m_detached == detached) {
        return;
    }

    m_detached = detached;
    Q_EMIT detachedChanged();
}

#include "moc_panelvisibility.cpp"
