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
 * Asks a local language model server (llama.cpp, Ollama or an OpenAI compatible
 * one) how the typed text continues, to suggest the next or current word.
 */
class LlmCompletionClient : public QObject
{
    Q_OBJECT

public:
    enum class Api {
        LlamaCpp,
        Ollama,
        OpenAiCompatible,
    };
    Q_ENUM(Api)

    explicit LlmCompletionClient(QObject *parent = nullptr);

    void setApi(Api api);
    void setEndpoint(const QUrl &endpoint);
    void setModel(const QString &model);
    void setApiKey(const QString &apiKey);

    /**
     * Requests a prediction for the text before the cursor, replacing any pending request.
     */
    void request(const QString &textBeforeCursor);
    void cancel();

    static QUrl requestUrl(Api api, const QUrl &endpoint);
    static QByteArray requestBody(Api api, const QString &model, const QString &prompt, int maxTokens);
    static QString parseContinuation(Api api, const QByteArray &response);

    /**
     * The word the continuation suggests: the completed current word, or the next word
     * if the text ends with a space. Empty if there is nothing useful.
     */
    static QString predictedWord(const QString &textBeforeCursor, const QString &continuation);

Q_SIGNALS:
    void predictionReady(const QString &textBeforeCursor, const QString &word);
    void requestFailed(const QString &error);

private:
    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    Api m_api = Api::LlamaCpp;
    QUrl m_endpoint;
    QString m_model;
    QString m_apiKey;
};
