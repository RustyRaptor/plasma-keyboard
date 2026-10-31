/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "fakeinputtext.h"

#include <QHash>

namespace
{
using Stroke = FakeInputText::Stroke;

// Printable characters of the US keymap that aren't letters
const QHash<char16_t, Stroke> SYMBOL_STROKES = {
    {u' ', {Qt::Key_Space, false}},
    {u'\n', {Qt::Key_Return, false}},
    {u'\t', {Qt::Key_Tab, false}},
    {u'-', {Qt::Key_Minus, false}},
    {u'_', {Qt::Key_Minus, true}},
    {u'=', {Qt::Key_Equal, false}},
    {u'+', {Qt::Key_Equal, true}},
    {u'[', {Qt::Key_BracketLeft, false}},
    {u'{', {Qt::Key_BracketLeft, true}},
    {u']', {Qt::Key_BracketRight, false}},
    {u'}', {Qt::Key_BracketRight, true}},
    {u'\\', {Qt::Key_Backslash, false}},
    {u'|', {Qt::Key_Backslash, true}},
    {u';', {Qt::Key_Semicolon, false}},
    {u':', {Qt::Key_Semicolon, true}},
    {u'\'', {Qt::Key_Apostrophe, false}},
    {u'"', {Qt::Key_Apostrophe, true}},
    {u',', {Qt::Key_Comma, false}},
    {u'<', {Qt::Key_Comma, true}},
    {u'.', {Qt::Key_Period, false}},
    {u'>', {Qt::Key_Period, true}},
    {u'/', {Qt::Key_Slash, false}},
    {u'?', {Qt::Key_Slash, true}},
    {u'`', {Qt::Key_QuoteLeft, false}},
    {u'~', {Qt::Key_QuoteLeft, true}},
    {u'!', {Qt::Key_1, true}},
    {u'@', {Qt::Key_2, true}},
    {u'#', {Qt::Key_3, true}},
    {u'$', {Qt::Key_4, true}},
    {u'%', {Qt::Key_5, true}},
    {u'^', {Qt::Key_6, true}},
    {u'&', {Qt::Key_7, true}},
    {u'*', {Qt::Key_8, true}},
    {u'(', {Qt::Key_9, true}},
    {u')', {Qt::Key_0, true}},
};
}

std::optional<FakeInputText::Stroke> FakeInputText::strokeForCharacter(QChar character)
{
    const char16_t c = character.unicode();
    if (c >= u'a' && c <= u'z') {
        return Stroke{Qt::Key_A + (c - u'a'), false};
    }
    if (c >= u'A' && c <= u'Z') {
        return Stroke{Qt::Key_A + (c - u'A'), true};
    }
    if (c >= u'0' && c <= u'9') {
        return Stroke{Qt::Key_0 + (c - u'0'), false};
    }

    const auto it = SYMBOL_STROKES.constFind(c);
    if (it != SYMBOL_STROKES.constEnd()) {
        return *it;
    }
    return std::nullopt;
}

QList<FakeInputText::Stroke> FakeInputText::strokesForText(const QString &text)
{
    QList<Stroke> strokes;
    strokes.reserve(text.size());
    for (const QChar character : text) {
        if (const auto stroke = strokeForCharacter(character)) {
            strokes.append(*stroke);
        }
    }
    return strokes;
}
