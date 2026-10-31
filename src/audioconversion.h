/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QByteArray>

class QAudioFormat;

namespace AudioConversion
{
/**
 * Sample rate expected by speech recognition models.
 */
constexpr int SPEECH_SAMPLE_RATE = 16000;

/**
 * Converts recorded audio to 16 kHz, mono, 16 bit PCM.
 */
QByteArray toSpeechPcm(const QByteArray &audio, const QAudioFormat &format);
}
