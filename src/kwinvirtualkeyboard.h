/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>

class QVariant;

/**
 * Talks to KWin's virtual-keyboard D-Bus interface.
 *
 * Since Plasma 6.8 KWin hides the input panel when the input method loses its
 * text-input context, even while the keyboard window stays mapped. A pinned
 * keyboard has no context, so it asks KWin to activate the input method again
 * and show the panel.
 *
 * Every call is a no-op when the interface is missing, which is the case on
 * compositors other than KWin and in the tests that run without a session bus.
 */
class KWinVirtualKeyboard : public QObject
{
    Q_OBJECT

public:
    explicit KWinVirtualKeyboard(QObject *parent = nullptr);

    /**
     * Whether the focused window has a text input the input method can type into.
     *
     * False for the context forceActivate() creates. Defaults to true when KWin
     * cannot be asked, so callers keep using the input-method protocol.
     */
    bool activeClientSupportsTextInput() const;

    /**
     * Whether KWin is currently showing the input panel.
     *
     * Defaults to true when KWin cannot be asked, so the panel is never forced
     * up on compositors that already keep a pinned keyboard visible.
     */
    bool panelVisible() const;

    /**
     * Asks KWin to activate the input method and show its panel without a text input.
     */
    void forceActivate() const;

private:
    QVariant property(const QString &name) const;
};
