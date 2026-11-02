/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "voiceinput.h"
#include "fakehttpserver.h"
#include "inputbackend.h"
#include "inputengine.h"
#include "transcriptionclient.h"

#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(PlasmaKeyboard, "org.kde.plasma.keyboard")

class CommitBackend : public InputBackend
{
public:
    bool isActive() const override
    {
        return true;
    }
    Qt::InputMethodHints inputMethodHints() const override
    {
        return Qt::ImhNoAutoUppercase;
    }
    QString surroundingText() const override
    {
        return {};
    }
    uint32_t cursorPositionUtf8() const override
    {
        return 0;
    }
    uint32_t anchorPositionUtf8() const override
    {
        return 0;
    }
    void setPreeditText(const QString &) override
    {
    }
    void commitText(const QString &text) override
    {
        commits.append(text);
    }
    void deleteSurroundingText(int, int) override
    {
    }
    bool sendKeyClick(int) override
    {
        return true;
    }
    bool sendKeyPressed(int, bool) override
    {
        return true;
    }

    QStringList commits;
};

// One second of 16 kHz, 16 bit silence
static const QByteArray ONE_SECOND(32000, '\0');

class VoiceInputTest : public QObject
{
    Q_OBJECT

private:
    void configure(VoiceInput &voiceInput, const FakeHttpServer &server)
    {
        voiceInput.transcriptionClient()->setApi(TranscriptionClient::Api::WhisperCpp);
        voiceInput.transcriptionClient()->setEndpoint(server.url());
        voiceInput.setAvailable(true);
    }

private Q_SLOTS:
    void commitsTranscription()
    {
        FakeHttpServer server;
        server.response = R"({"text":" Hello world."})";
        auto *backend = new CommitBackend;
        InputEngine engine(backend);
        VoiceInput voiceInput(&engine);
        configure(voiceInput, server);
        QSignalSpy stateSpy(&voiceInput, &VoiceInput::stateChanged);

        voiceInput.submitRecording(ONE_SECOND);
        QCOMPARE(voiceInput.state(), VoiceInput::Transcribing);

        QTRY_COMPARE(voiceInput.state(), VoiceInput::Idle);
        QCOMPARE(backend->commits, QStringList{u"Hello world."_s});
        QVERIFY(voiceInput.errorString().isEmpty());
    }

    void ignoresSilenceAndTooShortRecordings()
    {
        FakeHttpServer server;
        server.response = R"({"text":" [BLANK_AUDIO]"})";
        auto *backend = new CommitBackend;
        InputEngine engine(backend);
        VoiceInput voiceInput(&engine);
        configure(voiceInput, server);

        voiceInput.submitRecording(ONE_SECOND);
        QTRY_COMPARE(voiceInput.state(), VoiceInput::Idle);
        QVERIFY(backend->commits.isEmpty());

        // A tap on the microphone isn't worth a request
        voiceInput.submitRecording(QByteArray(1000, '\0'), 100ms);
        QCOMPARE(voiceInput.state(), VoiceInput::Idle);
        QCOMPARE(server.requests.size(), 1);
        QVERIFY(voiceInput.errorString().isEmpty());
    }

    void reportsRecordingsWithoutSound()
    {
        // A microphone that delivers nothing, unlike a tap, is a problem worth saying
        FakeHttpServer server;
        auto *backend = new CommitBackend;
        InputEngine engine(backend);
        VoiceInput voiceInput(&engine);
        configure(voiceInput, server);

        voiceInput.submitRecording(QByteArray(), 1500ms);

        QCOMPARE(voiceInput.state(), VoiceInput::Idle);
        QVERIFY(!voiceInput.errorString().isEmpty());
        QVERIFY(server.requests.isEmpty());
    }

    void reportsErrors()
    {
        auto *backend = new CommitBackend;
        InputEngine engine(backend);
        VoiceInput voiceInput(&engine);
        voiceInput.transcriptionClient()->setApi(TranscriptionClient::Api::WhisperCpp);
        voiceInput.transcriptionClient()->setEndpoint(QUrl(u"http://127.0.0.1:1"_s));
        voiceInput.setAvailable(true);

        voiceInput.submitRecording(ONE_SECOND);

        QTRY_COMPARE(voiceInput.state(), VoiceInput::Idle);
        QVERIFY(!voiceInput.errorString().isEmpty());
        QVERIFY(backend->commits.isEmpty());
    }

    void doesNothingWhenUnavailable()
    {
        auto *backend = new CommitBackend;
        InputEngine engine(backend);
        VoiceInput voiceInput(&engine);

        voiceInput.toggle();

        QCOMPARE(voiceInput.state(), VoiceInput::Idle);
    }

    void cancelDropsPendingTranscription()
    {
        FakeHttpServer server;
        server.response = R"({"text":"late"})";
        server.responseDelay = 200;
        auto *backend = new CommitBackend;
        InputEngine engine(backend);
        VoiceInput voiceInput(&engine);
        configure(voiceInput, server);

        voiceInput.submitRecording(ONE_SECOND);
        voiceInput.cancel();

        QCOMPARE(voiceInput.state(), VoiceInput::Idle);
        QTest::qWait(400);
        QVERIFY(backend->commits.isEmpty());
    }
};

QTEST_GUILESS_MAIN(VoiceInputTest)

#include "voiceinputtest.moc"
