/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "audioconversion.h"

#include <QAudioFormat>
#include <QList>

#include <algorithm>
#include <cstring>

namespace
{
// Reads one sample, scaled to the 16 bit range
template<typename T>
float readSample(const char *data)
{
    T value;
    std::memcpy(&value, data, sizeof(T));
    if constexpr (std::is_same_v<T, quint8>) {
        return (float(value) - 128) * 256;
    } else if constexpr (std::is_same_v<T, qint16>) {
        return value;
    } else if constexpr (std::is_same_v<T, qint32>) {
        return value / 65536.0f;
    } else {
        return value * 32767.0f;
    }
}

template<typename T>
QList<float> mixToMono(const QByteArray &audio, int channels)
{
    const qsizetype frameSize = sizeof(T) * channels;
    const qsizetype frames = audio.size() / frameSize;
    QList<float> mono(frames);
    for (qsizetype frame = 0; frame < frames; ++frame) {
        float sum = 0;
        for (int channel = 0; channel < channels; ++channel) {
            sum += readSample<T>(audio.constData() + frame * frameSize + channel * sizeof(T));
        }
        mono[frame] = sum / channels;
    }
    return mono;
}
}

QByteArray AudioConversion::toSpeechPcm(const QByteArray &audio, const QAudioFormat &format)
{
    const int channels = qMax(1, format.channelCount());
    if (format.sampleFormat() == QAudioFormat::Int16 && channels == 1 && format.sampleRate() == SPEECH_SAMPLE_RATE) {
        return audio;
    }

    QList<float> mono;
    switch (format.sampleFormat()) {
    case QAudioFormat::UInt8:
        mono = mixToMono<quint8>(audio, channels);
        break;
    case QAudioFormat::Int16:
        mono = mixToMono<qint16>(audio, channels);
        break;
    case QAudioFormat::Int32:
        mono = mixToMono<qint32>(audio, channels);
        break;
    case QAudioFormat::Float:
        mono = mixToMono<float>(audio, channels);
        break;
    default:
        return {};
    }

    // Linear interpolation to the speech sample rate
    const double step = double(format.sampleRate()) / SPEECH_SAMPLE_RATE;
    const qsizetype outputSize = qsizetype(mono.size() / step);
    QList<qint16> output(outputSize);
    for (qsizetype i = 0; i < outputSize; ++i) {
        const double position = i * step;
        const qsizetype index = qsizetype(position);
        const double fraction = position - index;
        const float next = index + 1 < mono.size() ? mono.at(index + 1) : mono.at(index);
        const double value = mono.at(index) + (next - mono.at(index)) * fraction;
        output[i] = qint16(std::clamp(value, -32767.0, 32767.0));
    }
    return QByteArray(reinterpret_cast<const char *>(output.constData()), output.size() * sizeof(qint16));
}
