/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QList>

/**
 * Remembers the keys the keyboard emulated, to recognize them when the
 * compositor hands them back through the input method's keyboard grab.
 */
class EmittedKeys
{
public:
    void note(quint32 key, bool pressed, qint64 timestampMs);

    /**
     * Whether a grabbed key event is the echo of an emulated key. Each emulated key matches once.
     */
    bool takeEcho(quint32 key, bool pressed, qint64 timestampMs);

private:
    struct Entry {
        quint32 key;
        bool pressed;
        qint64 timestampMs;
    };
    QList<Entry> m_entries;
};
