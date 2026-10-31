/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QList>
#include <QPointF>
#include <QStringList>

class WordDictionary;

/**
 * Turns a path swiped across the keyboard into the words it most likely spells.
 *
 * Compares the path to the ideal path through each candidate word's keys, by
 * location and by shape, and weighs in how common the word is.
 */
class GestureDecoder
{
public:
    /**
     * The center of each letter key, and the width of a key in the same units.
     */
    void setKeyCenters(const QHash<QChar, QPointF> &centers, qreal keyWidth);

    /**
     * The most likely words for the path, best first.
     */
    QStringList decode(const QList<QPointF> &path, const WordDictionary &dictionary, int limit) const;

private:
    QHash<QChar, QPointF> m_keyCenters;
    qreal m_keyWidth = 0;
};
