/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "llmcompletionclient.h"
#include "fakehttpserver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QTimer>

using namespace Qt::StringLiterals;

class LlmCompletionClientTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buildsRequestUrls()
    {
        const QUrl endpoint(u"http://localhost:8080/"_s);
        QCOMPARE(LlmCompletionClient::requestUrl(LlmCompletionClient::Api::LlamaCpp, endpoint), QUrl(u"http://localhost:8080/completion"_s));
        QCOMPARE(LlmCompletionClient::requestUrl(LlmCompletionClient::Api::Ollama, QUrl(u"http://localhost:11434"_s)),
                 QUrl(u"http://localhost:11434/api/generate"_s));
        QCOMPARE(LlmCompletionClient::requestUrl(LlmCompletionClient::Api::OpenAiCompatible, QUrl(u"http://host/v1"_s)), QUrl(u"http://host/v1/completions"_s));
        QCOMPARE(LlmCompletionClient::requestUrl(LlmCompletionClient::Api::OpenAiCompatible, QUrl(u"http://host"_s)), QUrl(u"http://host/v1/completions"_s));
    }

    void buildsRequestBodies()
    {
        const auto llama = QJsonDocument::fromJson(LlmCompletionClient::requestBody(LlmCompletionClient::Api::LlamaCpp, QString(), u"Hello wor"_s, 6)).object();
        QCOMPARE(llama[u"prompt"].toString(), u"Hello wor"_s);
        QCOMPARE(llama[u"n_predict"].toInt(), 6);
        QVERIFY(llama[u"cache_prompt"].toBool());

        const auto ollama = QJsonDocument::fromJson(LlmCompletionClient::requestBody(LlmCompletionClient::Api::Ollama, u"qwen3:0.6b"_s, u"Hi"_s, 6)).object();
        QCOMPARE(ollama[u"model"].toString(), u"qwen3:0.6b"_s);
        QCOMPARE(ollama[u"prompt"].toString(), u"Hi"_s);
        // Raw mode skips the chat template, so the model continues the text
        QVERIFY(ollama[u"raw"].toBool());
        QVERIFY(!ollama[u"stream"].toBool());
        QCOMPARE(ollama[u"options"].toObject()[u"num_predict"].toInt(), 6);

        const auto openAi = QJsonDocument::fromJson(LlmCompletionClient::requestBody(LlmCompletionClient::Api::OpenAiCompatible, u"m"_s, u"Hi"_s, 6)).object();
        QCOMPARE(openAi[u"model"].toString(), u"m"_s);
        QCOMPARE(openAi[u"max_tokens"].toInt(), 6);
    }

    void parsesResponses()
    {
        QCOMPARE(LlmCompletionClient::parseContinuation(LlmCompletionClient::Api::LlamaCpp, R"({"content":"ld peace"})"), u"ld peace"_s);
        QCOMPARE(LlmCompletionClient::parseContinuation(LlmCompletionClient::Api::Ollama, R"({"response":" there","done":true})"), u" there"_s);
        QCOMPARE(LlmCompletionClient::parseContinuation(LlmCompletionClient::Api::OpenAiCompatible, R"({"choices":[{"text":" friend"}]})"), u" friend"_s);
        QVERIFY(LlmCompletionClient::parseContinuation(LlmCompletionClient::Api::LlamaCpp, "not json").isEmpty());
    }

    void extractsPredictedWord_data()
    {
        QTest::addColumn<QString>("textBeforeCursor");
        QTest::addColumn<QString>("continuation");
        QTest::addColumn<QString>("expected");

        // Completing the word being typed
        QTest::newRow("completes word") << u"Hello wor"_s << u"ld, how are you"_s << u"world"_s;
        // Predicting the next word after a space
        QTest::newRow("next word") << u"Hello "_s << u"there friend"_s << u"there"_s;
        QTest::newRow("next word after leading space") << u"Hello"_s << u" there"_s << QString();
        QTest::newRow("leading space without prefix") << u"How are "_s << u" you"_s << u"you"_s;
        QTest::newRow("apostrophes") << u"I do"_s << u"n't know"_s << u"don't"_s;
        QTest::newRow("punctuation only") << u"Hello "_s << u"!"_s << QString();
        QTest::newRow("empty") << u"Hello "_s << QString() << QString();
    }

    void extractsPredictedWord()
    {
        QFETCH(QString, textBeforeCursor);
        QFETCH(QString, continuation);
        QFETCH(QString, expected);

        QCOMPARE(LlmCompletionClient::predictedWord(textBeforeCursor, continuation), expected);
    }

    void requestsPredictionFromServer()
    {
        FakeHttpServer server;
        server.response = R"({"content":"ld peace"})";
        LlmCompletionClient client;
        client.setApi(LlmCompletionClient::Api::LlamaCpp);
        client.setEndpoint(server.url());
        QSignalSpy readySpy(&client, &LlmCompletionClient::predictionReady);

        client.request(u"We want wor"_s);

        QVERIFY(readySpy.wait());
        QCOMPARE(readySpy.first().at(0).toString(), u"We want wor"_s);
        QCOMPARE(readySpy.first().at(1).toString(), u"world"_s);
        QVERIFY(server.requests.first().startsWith("POST /completion "));
    }

    void sendsApiKey()
    {
        FakeHttpServer server;
        server.response = R"({"choices":[{"text":"x"}]})";
        LlmCompletionClient client;
        client.setApi(LlmCompletionClient::Api::OpenAiCompatible);
        client.setEndpoint(server.url());
        client.setApiKey(u"secret"_s);
        QSignalSpy readySpy(&client, &LlmCompletionClient::predictionReady);

        client.request(u"a"_s);

        QVERIFY(readySpy.wait());
        QVERIFY(server.requests.first().contains("Authorization: Bearer secret"));
    }

    void onlyLatestRequestIsReported()
    {
        FakeHttpServer server;
        server.response = R"({"content":"ld"})";
        server.responseDelay = 100;
        LlmCompletionClient client;
        client.setApi(LlmCompletionClient::Api::LlamaCpp);
        client.setEndpoint(server.url());
        QSignalSpy readySpy(&client, &LlmCompletionClient::predictionReady);

        // Typing quickly replaces requests that are still in flight
        client.request(u"wo"_s);
        client.request(u"wor"_s);

        QVERIFY(readySpy.wait());
        QTest::qWait(200);
        QCOMPARE(readySpy.count(), 1);
        QCOMPARE(readySpy.first().at(0).toString(), u"wor"_s);
    }

    void reportsUnreachableServer()
    {
        LlmCompletionClient client;
        client.setApi(LlmCompletionClient::Api::Ollama);
        client.setEndpoint(QUrl(u"http://127.0.0.1:1"_s));
        QSignalSpy failedSpy(&client, &LlmCompletionClient::requestFailed);
        QSignalSpy readySpy(&client, &LlmCompletionClient::predictionReady);

        client.request(u"hello"_s);

        QVERIFY(failedSpy.wait());
        QCOMPARE(readySpy.count(), 0);
    }
};

QTEST_GUILESS_MAIN(LlmCompletionClientTest)

#include "llmcompletionclienttest.moc"
