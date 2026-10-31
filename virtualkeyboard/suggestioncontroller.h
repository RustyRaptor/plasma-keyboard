/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>

#include <memory>

class InputEngine;
class LlmCompletionClient;
class WordDictionary;

/**
 * Offers word suggestions in the candidate strip while typing.
 *
 * Suggestions come from the word dictionary, optionally led by the prediction of a
 * local language model. Words the user types are learned by the dictionary.
 */
class SuggestionController : public QObject
{
    Q_OBJECT

public:
    explicit SuggestionController(InputEngine *engine, QObject *parent = nullptr);
    ~SuggestionController() override;

    bool enabled() const;
    void setEnabled(bool enabled);

    void setDictionary(const std::shared_ptr<WordDictionary> &dictionary);

    /**
     * Uses a language model for predictions, or none if null.
     */
    void setLlmClient(LlmCompletionClient *client);

    /**
     * Delay after typing before asking the language model.
     */
    void setPredictionDelay(int milliseconds);

    /**
     * Shows a language model prediction, if it was made for the current text.
     */
    void applyPrediction(const QString &textBeforeCursor, const QString &word);

private:
    void update();

    InputEngine *m_engine = nullptr;
    std::shared_ptr<WordDictionary> m_dictionary;
    QPointer<LlmCompletionClient> m_llmClient;
    QTimer m_predictionTimer;
    QTimer m_saveTimer;
    bool m_enabled = false;
    QString m_textBeforeCursor;
    QString m_requestedText;
    QString m_predictionText;
    QString m_predictedWord;
    QString m_lastLearnedText;
};
