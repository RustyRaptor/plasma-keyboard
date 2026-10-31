/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "llmcompletionclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

static constexpr auto REQUEST_TIMEOUT = 5s;
static constexpr qsizetype MAXIMUM_PROMPT_LENGTH = 1000;
static constexpr int MAXIMUM_TOKENS = 6;

LlmCompletionClient::LlmCompletionClient(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

void LlmCompletionClient::setApi(Api api)
{
    m_api = api;
}

void LlmCompletionClient::setEndpoint(const QUrl &endpoint)
{
    m_endpoint = endpoint;
}

void LlmCompletionClient::setModel(const QString &model)
{
    m_model = model;
}

void LlmCompletionClient::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void LlmCompletionClient::request(const QString &textBeforeCursor)
{
    cancel();
    if (!m_endpoint.isValid()) {
        return;
    }

    QNetworkRequest request(requestUrl(m_api, m_endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_s);
    request.setTransferTimeout(REQUEST_TIMEOUT);
    if (!m_apiKey.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_apiKey.toUtf8());
    }

    const QString prompt = textBeforeCursor.right(MAXIMUM_PROMPT_LENGTH);
    QNetworkReply *reply = m_network->post(request, requestBody(m_api, m_model, prompt, MAXIMUM_TOKENS));
    m_reply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, textBeforeCursor] {
        reply->deleteLater();
        if (reply != m_reply) {
            // Replaced by a newer request
            return;
        }
        m_reply.clear();

        if (reply->error() != QNetworkReply::NoError) {
            Q_EMIT requestFailed(reply->errorString());
            return;
        }
        const QString continuation = parseContinuation(m_api, reply->readAll());
        Q_EMIT predictionReady(textBeforeCursor, predictedWord(textBeforeCursor, continuation));
    });
}

void LlmCompletionClient::cancel()
{
    if (QNetworkReply *reply = m_reply) {
        m_reply.clear();
        reply->abort();
    }
}

QUrl LlmCompletionClient::requestUrl(Api api, const QUrl &endpoint)
{
    QUrl url = endpoint;
    QString path = url.path();
    while (path.endsWith(u'/')) {
        path.chop(1);
    }

    switch (api) {
    case Api::LlamaCpp:
        path += u"/completion"_s;
        break;
    case Api::Ollama:
        path += u"/api/generate"_s;
        break;
    case Api::OpenAiCompatible:
        if (!path.endsWith(u"/v1"_s)) {
            path += u"/v1"_s;
        }
        path += u"/completions"_s;
        break;
    }
    url.setPath(path);
    return url;
}

QByteArray LlmCompletionClient::requestBody(Api api, const QString &model, const QString &prompt, int maxTokens)
{
    const QJsonArray stop{u"\n"_s};
    QJsonObject body;
    switch (api) {
    case Api::LlamaCpp:
        body = {
            {u"prompt"_s, prompt},
            {u"n_predict"_s, maxTokens},
            {u"temperature"_s, 0},
            {u"cache_prompt"_s, true},
            {u"stop"_s, stop},
        };
        break;
    case Api::Ollama:
        body = {
            {u"model"_s, model},
            {u"prompt"_s, prompt},
            // Continue the text as is instead of applying the chat template
            {u"raw"_s, true},
            {u"stream"_s, false},
            {u"keep_alive"_s, u"30m"_s},
            {u"options"_s, QJsonObject{{u"num_predict"_s, maxTokens}, {u"temperature"_s, 0}, {u"stop"_s, stop}}},
        };
        break;
    case Api::OpenAiCompatible:
        body = {
            {u"model"_s, model},
            {u"prompt"_s, prompt},
            {u"max_tokens"_s, maxTokens},
            {u"temperature"_s, 0},
            {u"stop"_s, stop},
        };
        break;
    }
    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

QString LlmCompletionClient::parseContinuation(Api api, const QByteArray &response)
{
    const QJsonObject object = QJsonDocument::fromJson(response).object();
    switch (api) {
    case Api::LlamaCpp:
        return object.value(u"content").toString();
    case Api::Ollama:
        return object.value(u"response").toString();
    case Api::OpenAiCompatible:
        return object.value(u"choices").toArray().first().toObject().value(u"text").toString();
    }
    return {};
}

static bool isWordCharacter(QChar character)
{
    return character.isLetterOrNumber() || character == u'\'';
}

static QString leadingWord(QStringView text)
{
    qsizetype end = 0;
    while (end < text.size() && isWordCharacter(text.at(end))) {
        ++end;
    }
    return text.left(end).toString();
}

QString LlmCompletionClient::predictedWord(const QString &textBeforeCursor, const QString &continuation)
{
    qsizetype prefixStart = textBeforeCursor.size();
    while (prefixStart > 0 && isWordCharacter(textBeforeCursor.at(prefixStart - 1))) {
        --prefixStart;
    }
    const QStringView prefix = QStringView(textBeforeCursor).sliced(prefixStart);

    if (!prefix.isEmpty()) {
        // A continuation starting with a space means the model considers the word complete
        const QString rest = leadingWord(continuation);
        return rest.isEmpty() ? QString() : prefix.toString() + rest;
    }
    return leadingWord(QStringView(continuation).trimmed());
}

#include "moc_llmcompletionclient.cpp"
