/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "transcriptionclient.h"
#include "fakehttpserver.h"

#include <QSignalSpy>
#include <QTest>
#include <QtEndian>

using namespace Qt::StringLiterals;

class TranscriptionClientTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void encodesWav()
    {
        const QByteArray pcm = QByteArray::fromHex("01000200ffff");

        const QByteArray wav = TranscriptionClient::wavFromPcm(pcm, 16000);

        QCOMPARE(wav.size(), 44 + pcm.size());
        QVERIFY(wav.startsWith("RIFF"));
        QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 4), quint32(36 + pcm.size()));
        QCOMPARE(wav.mid(8, 8), QByteArray("WAVEfmt "));
        // PCM, mono, 16 kHz, 16 bit
        QCOMPARE(qFromLittleEndian<quint16>(wav.constData() + 20), quint16(1));
        QCOMPARE(qFromLittleEndian<quint16>(wav.constData() + 22), quint16(1));
        QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 24), quint32(16000));
        QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 28), quint32(32000));
        QCOMPARE(qFromLittleEndian<quint16>(wav.constData() + 34), quint16(16));
        QCOMPARE(wav.mid(36, 4), QByteArray("data"));
        QCOMPARE(qFromLittleEndian<quint32>(wav.constData() + 40), quint32(pcm.size()));
        QCOMPARE(wav.mid(44), pcm);
    }

    void buildsRequestUrls()
    {
        QCOMPARE(TranscriptionClient::requestUrl(TranscriptionClient::Api::WhisperCpp, QUrl(u"http://localhost:8081/"_s)),
                 QUrl(u"http://localhost:8081/inference"_s));
        QCOMPARE(TranscriptionClient::requestUrl(TranscriptionClient::Api::OpenAiCompatible, QUrl(u"http://localhost:8000"_s)),
                 QUrl(u"http://localhost:8000/v1/audio/transcriptions"_s));
        QCOMPARE(TranscriptionClient::requestUrl(TranscriptionClient::Api::OpenAiCompatible, QUrl(u"http://localhost:8000/v1"_s)),
                 QUrl(u"http://localhost:8000/v1/audio/transcriptions"_s));
    }

    void parsesTranscription()
    {
        QCOMPARE(TranscriptionClient::parseTranscription(R"({"text":" Hello there.\n"})"), u"Hello there."_s);
        QVERIFY(TranscriptionClient::parseTranscription("garbage").isEmpty());
        // whisper.cpp marks silence with bracketed annotations
        QVERIFY(TranscriptionClient::parseTranscription(R"({"text":" [BLANK_AUDIO]"})").isEmpty());
    }

    void transcribesThroughWhisperCpp()
    {
        FakeHttpServer server;
        server.response = R"({"text":" Testing one two three."})";
        TranscriptionClient client;
        client.setApi(TranscriptionClient::Api::WhisperCpp);
        client.setEndpoint(server.url());
        client.setLanguage(u"en"_s);
        QSignalSpy readySpy(&client, &TranscriptionClient::transcriptionReady);

        client.transcribe(QByteArray(3200, '\0'), 16000);

        QVERIFY(readySpy.wait());
        QCOMPARE(readySpy.first().first().toString(), u"Testing one two three."_s);
        const QByteArray request = server.requests.first();
        QVERIFY(request.startsWith("POST /inference "));
        QVERIFY(request.contains("multipart/form-data"));
        QVERIFY(request.contains("name=\"file\"; filename=\"speech.wav\""));
        QVERIFY(request.contains("name=\"response_format\"\r\n\r\njson"));
        QVERIFY(request.contains("name=\"language\"\r\n\r\nen"));
    }

    void transcribesThroughOpenAiCompatibleServer()
    {
        FakeHttpServer server;
        server.response = R"({"text":"Hi"})";
        TranscriptionClient client;
        client.setApi(TranscriptionClient::Api::OpenAiCompatible);
        client.setEndpoint(server.url());
        client.setModel(u"Systran/faster-whisper-small"_s);
        client.setApiKey(u"key"_s);
        QSignalSpy readySpy(&client, &TranscriptionClient::transcriptionReady);

        client.transcribe(QByteArray(320, '\0'), 16000);

        QVERIFY(readySpy.wait());
        const QByteArray request = server.requests.first();
        QVERIFY(request.startsWith("POST /v1/audio/transcriptions "));
        QVERIFY(request.contains("name=\"model\"\r\n\r\nSystran/faster-whisper-small"));
        QVERIFY(request.contains("Authorization: Bearer key"));
        // No language means the server detects it
        QVERIFY(!request.contains("name=\"language\""));
    }

    void reportsFailures()
    {
        TranscriptionClient client;
        client.setApi(TranscriptionClient::Api::WhisperCpp);
        client.setEndpoint(QUrl(u"http://127.0.0.1:1"_s));
        QSignalSpy failedSpy(&client, &TranscriptionClient::transcriptionFailed);

        client.transcribe(QByteArray(320, '\0'), 16000);

        QVERIFY(failedSpy.wait());
        QVERIFY(!failedSpy.first().first().toString().isEmpty());
    }
};

QTEST_GUILESS_MAIN(TranscriptionClientTest)

#include "transcriptionclienttest.moc"
