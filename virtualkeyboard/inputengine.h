/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <qqmlintegration.h>

#include "textcomposers/abstracttextcomposer.h"
#include "wordcandidatelistmodel.h"

class InputBackend;

/**
 * The input context for the current session. This contains fields such as:
 * - Preedit text (word being built)
 * - Preedit prefix (character being formed, used for CJK)
 * - Capslock state
 * - Shift state
 * - Word prediction candidates list
 *
 * Text input is received from the TextComposer for the most part (though producers can also bypass it to enter text here).
 *
 * Events are sent and received via the InputBackend to communicate with the platform (which provides other state, such as the cursor position).
 */
class InputEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AbstractTextComposer *textComposer READ textComposer WRITE setTextComposer NOTIFY textComposerChanged)
    Q_PROPERTY(bool shiftActive READ shiftActive WRITE setShiftActive NOTIFY shiftActiveChanged)
    Q_PROPERTY(bool capsLockActive READ capsLockActive WRITE setCapsLockActive NOTIFY capsLockActiveChanged)
    Q_PROPERTY(bool uppercase READ uppercase NOTIFY uppercaseChanged)
    Q_PROPERTY(QString preeditText READ preeditText NOTIFY preeditTextChanged)
    Q_PROPERTY(QString preeditPrefix READ preeditPrefix NOTIFY preeditPrefixChanged)
    Q_PROPERTY(QStringList candidates READ candidates NOTIFY candidatesChanged)
    Q_PROPERTY(QString locale READ locale NOTIFY localeChanged)
    Q_PROPERTY(int anchorPosition READ anchorPosition NOTIFY anchorPositionChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(Qt::InputMethodHints inputMethodHints READ inputMethodHints NOTIFY inputMethodHintsChanged)
    Q_PROPERTY(QString surroundingText READ surroundingText NOTIFY surroundingTextChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectedTextChanged)
    Q_PROPERTY(QList<int> pressedKeys READ pressedKeys NOTIFY pressedKeysChanged)
    Q_PROPERTY(bool preeditBubbleVisibleHint READ preeditBubbleVisibleHint NOTIFY textComposerChanged)
    Q_PROPERTY(WordCandidateListModel *wordCandidateListModel READ wordCandidateListModel CONSTANT)
    Q_PROPERTY(bool wordCandidateListVisibleHint READ wordCandidateListVisibleHint NOTIFY wordCandidateListVisibleHintChanged)
    Q_PROPERTY(QString recentText READ recentText NOTIFY recentTextChanged)

public:
    explicit InputEngine(InputBackend *backend, QObject *parent = nullptr);

    AbstractTextComposer *textComposer() const;
    void setTextComposer(AbstractTextComposer *textComposer);
    bool shiftActive() const;
    void setShiftActive(bool active);
    bool capsLockActive() const;
    void setCapsLockActive(bool active);
    bool uppercase() const;
    QString preeditText() const;
    void setPreeditText(const QString &text);
    QString preeditPrefix() const;
    void setPreeditPrefix(const QString &text);
    QStringList candidates() const;
    void setCandidates(const QStringList &candidates);
    QString locale() const;
    void setLocale(const QString &locale);
    int anchorPosition() const;
    int cursorPosition() const;
    Qt::InputMethodHints inputMethodHints() const;
    QString surroundingText() const;
    QString selectedText() const;
    QList<int> pressedKeys() const;
    bool preeditBubbleVisibleHint() const;
    WordCandidateListModel *wordCandidateListModel() const;
    bool wordCandidateListVisibleHint() const;

    /** Sends a text key through the active text composer. */
    Q_INVOKABLE bool sendTextComposerKey(int key, const QString &text);

    /** Sends a key event directly to the backend, bypassing composition. */
    Q_INVOKABLE bool sendDirectKey(int key, bool pressed);
    Q_INVOKABLE bool isKeyPressed(int key) const;

    /**
     * Sends a key while holding modifiers, e.g. Ctrl+C. Modifiers that weren't
     * already held are released shortly after.
     */
    Q_INVOKABLE bool sendShortcut(int key, const QList<int> &modifiers);

    /**
     * Moves the text cursor by sending arrow keys, optionally extending the selection.
     */
    Q_INVOKABLE void moveCursor(int horizontalSteps, int verticalSteps, bool select);

    /**
     * Shows word suggestions as candidates, unless a composition owns the candidates.
     * Accepting one replaces replaceLength UTF-8 bytes before the cursor.
     */
    Q_INVOKABLE void setSuggestions(const QStringList &suggestions, int replaceLength);

    /**
     * Text typed through the keyboard since the cursor last moved, for clients without surrounding text.
     */
    QString recentText() const;
    Q_INVOKABLE bool selectCandidate(int index);
    Q_INVOKABLE void commit();
    Q_INVOKABLE void commit(const QString &text, int replaceFrom = 0, int replaceLength = 0);
    Q_INVOKABLE void clear();

    bool handleKeyCommit(int key, const QString &text);

Q_SIGNALS:
    void textComposerChanged();
    void shiftActiveChanged();
    void capsLockActiveChanged();
    void uppercaseChanged();
    void preeditTextChanged();
    void preeditPrefixChanged();
    void candidatesChanged();
    void localeChanged();
    void surroundingTextChanged();
    void selectedTextChanged();
    void anchorPositionChanged();
    void cursorPositionChanged();
    void inputMethodHintsChanged();
    void pressedKeysChanged();
    void wordCandidateListVisibleHintChanged();
    void suggestionAccepted(const QString &word);
    void recentTextChanged();

private:
    void connectBackend();
    void clearCompositionState(bool updateBackend);
    void resetShiftState();
    void updateAutomaticShift();
    bool sendDirectKeyClick(int key);
    void releaseKeysLater(const QList<int> &keys);
    void applyCandidates(const QStringList &candidates);
    void setRecentText(const QString &text);
    void trackRecentKey(int key);
    int utf16PositionFromUtf8Offset(uint32_t offset) const;

    InputBackend *m_backend = nullptr;
    QPointer<AbstractTextComposer> m_textComposer;
    mutable WordCandidateListModel *m_wordCandidateListModel = nullptr;
    QString m_locale;
    bool m_shiftActive = false;
    bool m_capsLockActive = false;
    QString m_preeditText;
    QString m_preeditPrefix;
    QStringList m_candidates;
    bool m_candidatesAreSuggestions = false;
    int m_suggestionReplaceLength = 0;
    QString m_recentText;
};
