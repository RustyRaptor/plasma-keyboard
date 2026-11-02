/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "emittedkeys.h"

// Echoes arrive within a few milliseconds; anything older is stale
static constexpr qint64 MAXIMUM_ECHO_DELAY_MS = 1000;

void EmittedKeys::note(quint32 key, bool pressed, qint64 timestampMs)
{
    m_entries.removeIf([timestampMs](const Entry &entry) {
        return timestampMs - entry.timestampMs > MAXIMUM_ECHO_DELAY_MS;
    });
    m_entries.append({key, pressed, timestampMs});
}

bool EmittedKeys::takeEcho(quint32 key, bool pressed, qint64 timestampMs)
{
    for (qsizetype i = 0; i < m_entries.size(); ++i) {
        const Entry &entry = m_entries.at(i);
        if (timestampMs - entry.timestampMs > MAXIMUM_ECHO_DELAY_MS) {
            continue;
        }
        if (entry.key == key && entry.pressed == pressed) {
            m_entries.removeAt(i);
            return true;
        }
    }
    return false;
}
