/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "suggestioncontroller.h"

#include "inputengine.h"
#include "llmcompletionclient.h"
#include "worddictionary.h"

using namespace std::chrono_literals;

namespace
{
constexpr int MAXIMUM_SUGGESTIONS = 3;
constexpr auto SAVE_DELAY = 10s;

// Fields where suggestions are unwanted or would leak sensitive text
constexpr Qt::InputMethodHints BLOCKING_HINTS = Qt::ImhSensitiveData | Qt::ImhHiddenText | Qt::ImhUrlCharactersOnly | Qt::ImhEmailCharactersOnly
    | Qt::ImhDigitsOnly | Qt::ImhFormattedNumbersOnly | Qt::ImhDialableCharactersOnly;

bool isWordCharacter(QChar character)
{
    return character.isLetterOrNumber() || character == u'\'';
}

// The word the text ends with, possibly empty
QString trailingWord(const QString &text)
{
    qsizetype start = text.size();
    while (start > 0 && isWordCharacter(text.at(start - 1))) {
        --start;
    }
    return text.sliced(start);
}
}

SuggestionController::SuggestionController(InputEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    Q_ASSERT(m_engine);

    m_predictionTimer.setSingleShot(true);
    m_predictionTimer.setInterval(200);
    connect(&m_predictionTimer, &QTimer::timeout, this, [this] {
        if (m_llmClient && m_requestedText != m_textBeforeCursor) {
            m_requestedText = m_textBeforeCursor;
            m_llmClient->request(m_textBeforeCursor);
        }
    });

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(SAVE_DELAY);
    connect(&m_saveTimer, &QTimer::timeout, this, [this] {
        if (m_dictionary) {
            m_dictionary->saveLearnedWords();
        }
    });

    connect(m_engine, &InputEngine::surroundingTextChanged, this, &SuggestionController::update);
    connect(m_engine, &InputEngine::recentTextChanged, this, &SuggestionController::update);
    connect(m_engine, &InputEngine::inputMethodHintsChanged, this, &SuggestionController::update);
    connect(m_engine, &InputEngine::preeditTextChanged, this, &SuggestionController::update);
}

SuggestionController::~SuggestionController()
{
    if (m_saveTimer.isActive() && m_dictionary) {
        m_dictionary->saveLearnedWords();
    }
}

bool SuggestionController::enabled() const
{
    return m_enabled;
}

void SuggestionController::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    update();
}

void SuggestionController::setDictionary(const std::shared_ptr<WordDictionary> &dictionary)
{
    m_dictionary = dictionary;
    update();
}

void SuggestionController::setLlmClient(LlmCompletionClient *client)
{
    if (m_llmClient) {
        disconnect(m_llmClient, nullptr, this, nullptr);
        m_llmClient->cancel();
    }

    m_llmClient = client;
    m_requestedText.clear();
    m_predictedWord.clear();
    if (m_llmClient) {
        connect(m_llmClient, &LlmCompletionClient::predictionReady, this, &SuggestionController::applyPrediction);
    }
    update();
}

void SuggestionController::setPredictionDelay(int milliseconds)
{
    m_predictionTimer.setInterval(milliseconds);
}

void SuggestionController::applyPrediction(const QString &textBeforeCursor, const QString &word)
{
    m_predictionText = textBeforeCursor;
    m_predictedWord = word;
    if (textBeforeCursor == m_textBeforeCursor) {
        update();
    }
}

void SuggestionController::update()
{
    const bool blocked = m_engine->inputMethodHints().testAnyFlags(BLOCKING_HINTS);
    if (!m_enabled || blocked) {
        m_predictionTimer.stop();
        m_textBeforeCursor.clear();
        m_engine->setSuggestions({}, 0);
        return;
    }

    const QString surroundingText = m_engine->surroundingText();
    m_textBeforeCursor = surroundingText.isEmpty() ? m_engine->recentText() : surroundingText.left(m_engine->cursorPosition());
    const QString word = trailingWord(m_textBeforeCursor);

    // Learn a word once it's finished, e.g. by a space
    if (word.isEmpty() && m_dictionary && m_textBeforeCursor != m_lastLearnedText) {
        m_lastLearnedText = m_textBeforeCursor;
        const QString finishedWord = trailingWord(m_textBeforeCursor.trimmed());
        if (!finishedWord.isEmpty() && m_textBeforeCursor.trimmed().endsWith(finishedWord)) {
            m_dictionary->learn(finishedWord);
            m_saveTimer.start();
        }
    }

    QStringList suggestions;
    if (m_predictionText == m_textBeforeCursor && !m_predictedWord.isEmpty()) {
        suggestions.append(m_predictedWord);
    }
    if (m_dictionary && !word.isEmpty()) {
        const QStringList completions = m_dictionary->completions(word, MAXIMUM_SUGGESTIONS);
        for (const QString &completion : completions) {
            if (!suggestions.contains(completion, Qt::CaseInsensitive)) {
                suggestions.append(completion);
            }
        }
    }
    m_engine->setSuggestions(suggestions.mid(0, MAXIMUM_SUGGESTIONS), word.toUtf8().size());

    if (m_llmClient && !m_textBeforeCursor.trimmed().isEmpty() && m_requestedText != m_textBeforeCursor) {
        m_predictionTimer.start();
    }
}

#include "moc_suggestioncontroller.cpp"
