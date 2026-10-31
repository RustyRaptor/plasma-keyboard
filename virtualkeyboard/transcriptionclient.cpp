/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "transcriptionclient.h"

#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QtEndian>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

static constexpr auto REQUEST_TIMEOUT = 60s;

TranscriptionClient::TranscriptionClient(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void TranscriptionClient::setApi(Api api)
{
    m_api = api;
}

void TranscriptionClient::setEndpoint(const QUrl &endpoint)
{
    m_endpoint = endpoint;
}

void TranscriptionClient::setModel(const QString &model)
{
    m_model = model;
}

void TranscriptionClient::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void TranscriptionClient::setLanguage(const QString &language)
{
    m_language = language;
}

void TranscriptionClient::transcribe(const QByteArray &pcm, int sampleRate)
{
    cancel();

    QNetworkRequest request(requestUrl(m_api, m_endpoint));
    request.setTransferTimeout(REQUEST_TIMEOUT);
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    }

    auto *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    const auto addField = [multiPart](const QString &name, const QByteArray &value) {
        QHttpPart part;
        part.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"%1\""_s.arg(name));
        part.setBody(value);
        multiPart->append(part);
    };

    QHttpPart file;
    file.setHeader(QNetworkRequest::ContentTypeHeader, u"audio/wav"_s);
    file.setHeader(QNetworkRequest::ContentDispositionHeader, u"form-data; name=\"file\"; filename=\"speech.wav\""_s);
    file.setBody(wavFromPcm(pcm, sampleRate));
    multiPart->append(file);
    addField(u"response_format"_s, "json");
    if (!m_model.isEmpty()) {
        addField(u"model"_s, m_model.toUtf8());
    }
    if (!m_language.isEmpty()) {
        addField(u"language"_s, m_language.toUtf8());
    }

    QNetworkReply *reply = m_network->post(request, multiPart);
    multiPart->setParent(reply);
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply != m_reply) {
            return;
        }
        m_reply.clear();

        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT transcriptionFailed(reply->errorString());
            return;
        }
        Q_EMIT transcriptionReady(parseTranscription(reply->readAll()));
    });
}

void TranscriptionClient::cancel()
{
    if (QNetworkReply *reply = m_reply) {
        m_reply.clear();
        reply->abort();
    }
}

QByteArray TranscriptionClient::wavFromPcm(const QByteArray &pcm, int sampleRate)
{
    constexpr quint16 channels = 1;
    constexpr quint16 bitsPerSample = 16;
    constexpr quint16 blockAlign = channels * bitsPerSample / 8;

    QByteArray wav;
    wav.reserve(44 + pcm.size());
    const auto append32 = [&wav](quint32 value) {
        char bytes[4];
        qToLittleEndian(value, bytes);
        wav.append(bytes, 4);
    };
    const auto append16 = [&wav](quint16 value) {
        char bytes[2];
        qToLittleEndian(value, bytes);
        wav.append(bytes, 2);
    };

    wav.append("RIFF");
    append32(36 + pcm.size());
    wav.append("WAVEfmt ");
    append32(16);
    append16(1); // PCM
    append16(channels);
    append32(sampleRate);
    append32(sampleRate * blockAlign);
    append16(blockAlign);
    append16(bitsPerSample);
    wav.append("data");
    append32(pcm.size());
    wav.append(pcm);
    return wav;
}

QUrl TranscriptionClient::requestUrl(Api api, const QUrl &endpoint)
{
    QUrl url = endpoint;
    QString path = url.path();
    while (path.endsWith(u'/')) {
        path.chop(1);
    }

    switch (api) {
    case Api::WhisperCpp:
        path += u"/inference"_s;
        break;
    case Api::OpenAiCompatible:
        if (!path.endsWith(u"/v1"_s)) {
            path += u"/v1"_s;
        }
        path += u"/audio/transcriptions"_s;
        break;
    }
    url.setPath(path);
    return url;
}

QString TranscriptionClient::parseTranscription(const QByteArray &response)
{
    QString text = QJsonDocument::fromJson(response).object().value(u"text").toString();
    // Whisper annotates non-speech, e.g. "[BLANK_AUDIO]" or "(music)"
    static const QRegularExpression annotations(u"\\[[^\\]]*\\]|\\([^)]*\\)"_s);
    text.remove(annotations);
    return text.simplified();
}

#include "moc_transcriptionclient.cpp"
