/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

/**
 * Sends recorded speech to a speech recognition server (whisper.cpp or an
 * OpenAI compatible one, e.g. faster-whisper-server) and returns the text.
 */
class TranscriptionClient : public QObject
{
    Q_OBJECT

public:
    enum class Api {
        WhisperCpp,
        OpenAiCompatible,
    };
    Q_ENUM(Api)

    explicit TranscriptionClient(QObject *parent = nullptr);

    void setApi(Api api);
    void setEndpoint(const QUrl &endpoint);
    void setModel(const QString &model);
    void setApiKey(const QString &apiKey);

    /**
     * Language code such as "en", or empty to let the server detect it.
     */
    void setLanguage(const QString &language);

    /**
     * Transcribes 16 bit mono PCM audio.
     */
    void transcribe(const QByteArray &pcm, int sampleRate);
    void cancel();

    static QByteArray wavFromPcm(const QByteArray &pcm, int sampleRate);
    static QUrl requestUrl(Api api, const QUrl &endpoint);
    static QString parseTranscription(const QByteArray &response);

Q_SIGNALS:
    void transcriptionReady(const QString &text);
    void transcriptionFailed(const QString &error);

private:
    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    Api m_api = Api::WhisperCpp;
    QUrl m_endpoint;
    QString m_model;
    QString m_apiKey;
    QString m_language;
};
