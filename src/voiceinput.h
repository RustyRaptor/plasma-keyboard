/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAudioFormat>
#include <QElapsedTimer>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <qqmlintegration.h>

class InputEngine;
class QAudioSource;
class QIODevice;
class TranscriptionClient;

/**
 * Types what the user says: records from the microphone, has a speech
 * recognition server transcribe it and commits the text.
 */
class VoiceInput : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by InputMethodConnection")
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    enum State {
        Idle,
        Recording,
        Transcribing,
    };
    Q_ENUM(State)

    explicit VoiceInput(InputEngine *engine, QObject *parent = nullptr);
    ~VoiceInput() override;

    bool available() const;
    void setAvailable(bool available);
    State state() const;
    QString errorString() const;

    TranscriptionClient *transcriptionClient() const;

    /**
     * Starts recording when idle, finishes it when recording, cancels when transcribing.
     */
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void cancel();

    /**
     * Transcribes 16 kHz, mono, 16 bit audio recorded for the given time.
     *
     * Too little audio from a short recording is an accidental tap and is ignored, from a longer one it's reported
     * as an error.
     */
    void submitRecording(const QByteArray &pcm, std::chrono::milliseconds recordingTime = std::chrono::milliseconds::zero());

Q_SIGNALS:
    void availableChanged();
    void stateChanged();
    void errorStringChanged();

private:
    void startRecording();
    void stopRecording();
    void setState(State state);
    void setErrorString(const QString &error);

    InputEngine *m_engine = nullptr;
    TranscriptionClient *m_client = nullptr;
    QAudioSource *m_source = nullptr;
    QElapsedTimer m_recordingTimer;
    QPointer<QIODevice> m_device;
    QAudioFormat m_format;
    QByteArray m_audio;
    QTimer m_limitTimer;
    bool m_available = false;
    State m_state = Idle;
    QString m_errorString;
};
