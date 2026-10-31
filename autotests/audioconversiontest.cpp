/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "audioconversion.h"

#include <QAudioFormat>
#include <QTest>

static QByteArray int16Samples(const QList<qint16> &samples)
{
    return QByteArray(reinterpret_cast<const char *>(samples.constData()), samples.size() * sizeof(qint16));
}

static QList<qint16> toSamples(const QByteArray &pcm)
{
    QList<qint16> samples(pcm.size() / sizeof(qint16));
    memcpy(samples.data(), pcm.constData(), samples.size() * sizeof(qint16));
    return samples;
}

class AudioConversionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsMatchingFormat()
    {
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        const QByteArray input = int16Samples({1, -2, 300});

        QCOMPARE(AudioConversion::toSpeechPcm(input, format), input);
    }

    void downmixesStereo()
    {
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(2);
        format.setSampleFormat(QAudioFormat::Int16);

        const QByteArray output = AudioConversion::toSpeechPcm(int16Samples({100, 300, -100, -300}), format);

        QCOMPARE(toSamples(output), QList<qint16>({200, -200}));
    }

    void convertsFloatSamples()
    {
        QAudioFormat format;
        format.setSampleRate(16000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Float);
        const QList<float> samples = {0.5f, -1.0f, 2.0f};
        const QByteArray input(reinterpret_cast<const char *>(samples.constData()), samples.size() * sizeof(float));

        const QList<qint16> output = toSamples(AudioConversion::toSpeechPcm(input, format));

        // Out of range samples are clipped
        QCOMPARE(output, QList<qint16>({16383, -32767, 32767}));
    }

    void resamplesToSixteenKilohertz()
    {
        QAudioFormat format;
        format.setSampleRate(48000);
        format.setChannelCount(1);
        format.setSampleFormat(QAudioFormat::Int16);
        QList<qint16> input;
        for (int i = 0; i < 4800; ++i) {
            input.append(qint16(i));
        }

        const QList<qint16> output = toSamples(AudioConversion::toSpeechPcm(int16Samples(input), format));

        // A tenth of a second at 16 kHz, following the ramp
        QCOMPARE(output.size(), 1600);
        QCOMPARE(output.at(0), qint16(0));
        QCOMPARE(output.at(100), qint16(300));
        QCOMPARE(output.at(1000), qint16(3000));
    }
};

QTEST_GUILESS_MAIN(AudioConversionTest)

#include "audioconversiontest.moc"
