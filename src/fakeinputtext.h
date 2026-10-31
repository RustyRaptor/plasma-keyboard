/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QList>
#include <QString>

#include <optional>

/**
 * Translates text into key strokes for keyboard emulation.
 *
 * Used when there is no input context to commit text to, so the keyboard
 * has to emulate key presses instead. Assumes a US keymap.
 */
namespace FakeInputText
{
struct Stroke {
    int key = 0;
    bool shift = false;
};

std::optional<Stroke> strokeForCharacter(QChar character);
QList<Stroke> strokesForText(const QString &text);
}
