/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "voiceinput.h"

#include "audioconversion.h"
#include "inputengine.h"
#include "transcriptionclient.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QMediaDevices>

using namespace std::chrono_literals;

// Recordings shorter than this are accidental taps on the microphone button
static constexpr qsizetype MINIMUM_RECORDING_BYTES = AudioConversion::SPEECH_SAMPLE_RATE * 2 / 4;
// Whisper works on up to 30 second windows, longer recordings get slow
static constexpr auto MAXIMUM_RECORDING_TIME = 30s;

VoiceInput::VoiceInput(InputEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
    , m_client(new TranscriptionClient(this))
{
    m_limitTimer.setSingleShot(true);
    m_limitTimer.setInterval(MAXIMUM_RECORDING_TIME);
    connect(&m_limitTimer, &QTimer::timeout, this, &VoiceInput::stopRecording);

    connect(m_client, &TranscriptionClient::transcriptionReady, this, [this](const QString &text) {
        if (m_state != Transcribing) {
            return;
        }
        setState(Idle);
        if (!text.isEmpty()) {
            m_engine->commitWord(text, {});
        }
    });
    connect(m_client, &TranscriptionClient::transcriptionFailed, this, [this](const QString &error) {
        if (m_state != Transcribing) {
            return;
        }
        setErrorString(error);
        setState(Idle);
    });
}

VoiceInput::~VoiceInput()
{
    if (m_source) {
        m_source->stop();
    }
}

bool VoiceInput::available() const
{
    return m_available;
}

void VoiceInput::setAvailable(bool available)
{
    if (m_available == available) {
        return;
    }

    m_available = available;
    if (!m_available) {
        cancel();
    }
    Q_EMIT availableChanged();
}

VoiceInput::State VoiceInput::state() const
{
    return m_state;
}

QString VoiceInput::errorString() const
{
    return m_errorString;
}

TranscriptionClient *VoiceInput::transcriptionClient() const
{
    return m_client;
}

void VoiceInput::toggle()
{
    if (!m_available) {
        return;
    }

    switch (m_state) {
    case Idle:
        startRecording();
        break;
    case Recording:
        stopRecording();
        break;
    case Transcribing:
        cancel();
        break;
    }
}

void VoiceInput::cancel()
{
    m_limitTimer.stop();
    if (m_source) {
        m_source->stop();
    }
    m_audio.clear();
    m_client->cancel();
    setState(Idle);
}

void VoiceInput::submitRecording(const QByteArray &pcm)
{
    setErrorString(QString());
    if (pcm.size() < MINIMUM_RECORDING_BYTES) {
        setState(Idle);
        return;
    }

    setState(Transcribing);
    m_client->transcribe(pcm, AudioConversion::SPEECH_SAMPLE_RATE);
}

void VoiceInput::startRecording()
{
    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        setErrorString(tr("No microphone found"));
        return;
    }

    QAudioFormat format;
    format.setSampleRate(AudioConversion::SPEECH_SAMPLE_RATE);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    m_format = device.isFormatSupported(format) ? format : device.preferredFormat();

    delete m_source;
    m_source = new QAudioSource(device, m_format, this);
    m_audio.clear();
    setErrorString(QString());

    m_device = m_source->start();
    if (!m_device) {
        setErrorString(tr("Could not start recording"));
        return;
    }
    connect(m_device, &QIODevice::readyRead, this, [this] {
        if (m_device) {
            m_audio += m_device->readAll();
        }
    });

    m_limitTimer.start();
    setState(Recording);
}

void VoiceInput::stopRecording()
{
    if (m_state != Recording || !m_source) {
        return;
    }

    m_limitTimer.stop();
    if (m_device) {
        m_audio += m_device->readAll();
    }
    m_source->stop();

    const QByteArray pcm = AudioConversion::toSpeechPcm(m_audio, m_format);
    m_audio.clear();
    submitRecording(pcm);
}

void VoiceInput::setState(State state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    Q_EMIT stateChanged();
}

void VoiceInput::setErrorString(const QString &error)
{
    if (m_errorString == error) {
        return;
    }

    m_errorString = error;
    Q_EMIT errorStringChanged();
}

#include "moc_voiceinput.cpp"
