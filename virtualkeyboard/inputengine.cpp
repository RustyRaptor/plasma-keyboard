/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "inputengine.h"

#include "inputbackend.h"
#include "textcomposers/abstracttextcomposer.h"

#include <QLocale>
#include <QTimer>

using namespace std::chrono_literals;

static constexpr auto MODIFIER_RELEASE_DELAY = 20ms;

static bool isNonShiftModifierKey(int key)
{
    switch (key) {
    case Qt::Key_Control:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
        return true;
    default:
        return false;
    }
}

InputEngine::InputEngine(InputBackend *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_locale(QLocale().name())
{
    Q_ASSERT(m_backend);
    connectBackend();
    connect(this, &InputEngine::uppercaseChanged, this, [this] {
        if (!m_textComposer) {
            return;
        }
        m_textComposer->setTextCase(uppercase() ? AbstractTextComposer::TextCase::Upper : AbstractTextComposer::TextCase::Lower);
    });
    connect(this, &InputEngine::textComposerChanged, this, [this] {
        if (!m_textComposer) {
            return;
        }
        m_textComposer->setTextCase(uppercase() ? AbstractTextComposer::TextCase::Upper : AbstractTextComposer::TextCase::Lower);
    });
    connect(this, &InputEngine::preeditTextChanged, this, &InputEngine::updateAutomaticShift);
}

AbstractTextComposer *InputEngine::textComposer() const
{
    return m_textComposer;
}

void InputEngine::setTextComposer(AbstractTextComposer *textComposer)
{
    if (m_textComposer == textComposer) {
        return;
    }

    if (m_textComposer) {
        disconnect(m_textComposer, nullptr, this, nullptr);
        m_textComposer->reset();
    }

    m_textComposer = textComposer;
    if (m_textComposer) {
        m_textComposer->setInputEngine(this);
        m_textComposer->setTextCase(uppercase() ? AbstractTextComposer::TextCase::Upper : AbstractTextComposer::TextCase::Lower);
        connect(m_textComposer, &AbstractTextComposer::autoCapitalizationSupportedChanged, this, &InputEngine::resetShiftState);
        connect(m_textComposer, &AbstractTextComposer::autoCapitalizationPolicyChanged, this, &InputEngine::updateAutomaticShift);

        connect(m_textComposer, &QObject::destroyed, this, [this] {
            clear();
            Q_EMIT textComposerChanged();
        });
    }
    Q_EMIT textComposerChanged();

    if (!m_textComposer || !m_textComposer->autoCapitalizationSupported()) {
        setCapsLockActive(false);
        setShiftActive(false);
    } else if (inputMethodHints().testAnyFlags(Qt::ImhPreferUppercase | Qt::ImhUppercaseOnly)) {
        resetShiftState();
    } else {
        updateAutomaticShift();
    }
}

bool InputEngine::shiftActive() const
{
    return m_shiftActive;
}

void InputEngine::setShiftActive(bool active)
{
    if (m_shiftActive == active) {
        return;
    }

    m_shiftActive = active;
    if (!m_shiftActive && m_backend->pressedKeys().contains(Qt::Key_Shift)) {
        m_backend->sendKeyPressed(Qt::Key_Shift, false);
    }
    Q_EMIT shiftActiveChanged();
    Q_EMIT uppercaseChanged();
}

bool InputEngine::capsLockActive() const
{
    return m_capsLockActive;
}

void InputEngine::setCapsLockActive(bool active)
{
    if (m_capsLockActive == active) {
        return;
    }

    m_capsLockActive = active;
    Q_EMIT capsLockActiveChanged();
    Q_EMIT uppercaseChanged();
}

bool InputEngine::uppercase() const
{
    if (m_capsLockActive) {
        return true;
    }
    if (!m_shiftActive) {
        return false;
    }

    for (const int key : m_backend->pressedKeys()) {
        if (isNonShiftModifierKey(key)) {
            return false;
        }
    }

    return true;
}

QString InputEngine::preeditText() const
{
    return m_preeditText;
}

void InputEngine::setPreeditText(const QString &text)
{
    if (m_preeditText == text) {
        return;
    }

    m_preeditText = text;
    m_backend->setPreeditText(text);
    Q_EMIT preeditTextChanged();
}

QString InputEngine::preeditPrefix() const
{
    return m_preeditPrefix;
}

void InputEngine::setPreeditPrefix(const QString &text)
{
    if (m_preeditPrefix == text) {
        return;
    }

    m_preeditPrefix = text;
    Q_EMIT preeditPrefixChanged();
}

QStringList InputEngine::candidates() const
{
    return m_candidates;
}

void InputEngine::setCandidates(const QStringList &candidates)
{
    m_candidatesAreSuggestions = false;
    applyCandidates(candidates);
}

void InputEngine::applyCandidates(const QStringList &candidates)
{
    if (m_candidates == candidates) {
        return;
    }

    m_candidates = candidates;
    Q_EMIT candidatesChanged();
    Q_EMIT wordCandidateListVisibleHintChanged();
}

QString InputEngine::locale() const
{
    return m_locale;
}

void InputEngine::setLocale(const QString &locale)
{
    if (m_locale == locale) {
        return;
    }

    m_locale = locale;
    Q_EMIT localeChanged();
}

int InputEngine::anchorPosition() const
{
    return utf16PositionFromUtf8Offset(m_backend->anchorPositionUtf8());
}

int InputEngine::cursorPosition() const
{
    return utf16PositionFromUtf8Offset(m_backend->cursorPositionUtf8());
}

Qt::InputMethodHints InputEngine::inputMethodHints() const
{
    return m_backend->inputMethodHints();
}

QString InputEngine::surroundingText() const
{
    return m_backend->surroundingText();
}

QString InputEngine::selectedText() const
{
    const QByteArray utf8 = surroundingText().toUtf8();
    const int start = qMin(m_backend->cursorPositionUtf8(), m_backend->anchorPositionUtf8());
    const int end = qMax(m_backend->cursorPositionUtf8(), m_backend->anchorPositionUtf8());
    return QString::fromUtf8(utf8.mid(start, end - start));
}

QList<int> InputEngine::pressedKeys() const
{
    return m_backend->pressedKeys();
}

bool InputEngine::preeditBubbleVisibleHint() const
{
    return m_textComposer && m_textComposer->showsPreeditBubble();
}

WordCandidateListModel *InputEngine::wordCandidateListModel() const
{
    if (!m_wordCandidateListModel) {
        m_wordCandidateListModel = new WordCandidateListModel(const_cast<InputEngine *>(this), const_cast<InputEngine *>(this));
    }
    return m_wordCandidateListModel;
}

bool InputEngine::wordCandidateListVisibleHint() const
{
    return !m_candidates.isEmpty();
}

bool InputEngine::sendTextComposerKey(int key, const QString &text)
{
    if (!m_textComposer) {
        return false;
    }

    if (!m_backend->pressedKeys().isEmpty()) {
        return sendDirectKeyClick(key);
    }

    return m_textComposer->keyEvent(Qt::Key(key), text);
}

bool InputEngine::sendDirectKey(int key, bool pressed)
{
    if (pressed) {
        trackRecentKey(key);
    }
    return m_backend->sendKeyPressed(key, pressed);
}

void InputEngine::trackRecentKey(int key)
{
    switch (key) {
    case Qt::Key_Backspace:
        setRecentText(m_recentText.chopped(qMin<qsizetype>(1, m_recentText.size())));
        break;
    case Qt::Key_Space:
        setRecentText(m_recentText + u' ');
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        setRecentText(m_recentText + u'\n');
        break;
    case Qt::Key_Tab:
        setRecentText(m_recentText + u'\t');
        break;
    case Qt::Key_Delete:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        setRecentText(QString());
        break;
    default:
        break;
    }
}

bool InputEngine::isKeyPressed(int key) const
{
    return m_backend->pressedKeys().contains(key);
}

bool InputEngine::sendShortcut(int key, const QList<int> &modifiers)
{
    QList<int> pressedModifiers;
    for (const int modifier : modifiers) {
        if (m_backend->pressedKeys().contains(modifier)) {
            continue;
        }
        if (!m_backend->sendKeyPressed(modifier, true)) {
            releaseKeysLater(pressedModifiers);
            return false;
        }
        pressedModifiers.append(modifier);
    }

    const bool sent = m_backend->sendKeyPressed(key, true) && m_backend->sendKeyPressed(key, false);
    releaseKeysLater(pressedModifiers);
    return sent;
}

void InputEngine::moveCursor(int horizontalSteps, int verticalSteps, bool select)
{
    if (horizontalSteps == 0 && verticalSteps == 0) {
        return;
    }

    commit();
    setRecentText(QString());

    QList<int> pressedModifiers;
    if (select && !m_backend->pressedKeys().contains(Qt::Key_Shift) && m_backend->sendKeyPressed(Qt::Key_Shift, true)) {
        pressedModifiers.append(Qt::Key_Shift);
    }

    const int horizontalKey = horizontalSteps > 0 ? Qt::Key_Right : Qt::Key_Left;
    for (int i = 0; i < qAbs(horizontalSteps); ++i) {
        m_backend->sendKeyClick(horizontalKey);
    }
    const int verticalKey = verticalSteps > 0 ? Qt::Key_Down : Qt::Key_Up;
    for (int i = 0; i < qAbs(verticalSteps); ++i) {
        m_backend->sendKeyClick(verticalKey);
    }

    releaseKeysLater(pressedModifiers);
}

void InputEngine::releaseKeysLater(const QList<int> &keys)
{
    if (keys.isEmpty()) {
        return;
    }

    // Give the client a moment to see the modifiers before releasing them
    QTimer::singleShot(MODIFIER_RELEASE_DELAY, this, [this, keys] {
        for (const int key : keys) {
            if (m_backend->pressedKeys().contains(key)) {
                m_backend->sendKeyPressed(key, false);
            }
        }
    });
}

void InputEngine::setSuggestions(const QStringList &suggestions, int replaceLength)
{
    // A composition in progress owns the candidate list
    if (!m_preeditText.isEmpty() || (!m_candidatesAreSuggestions && !m_candidates.isEmpty())) {
        return;
    }

    m_suggestionReplaceLength = replaceLength;
    applyCandidates(suggestions);
    m_candidatesAreSuggestions = !suggestions.isEmpty();
}

QString InputEngine::recentText() const
{
    return m_recentText;
}

void InputEngine::setRecentText(const QString &text)
{
    // Only the end of the text is useful as context
    constexpr qsizetype maximumLength = 500;
    const QString capped = text.size() > maximumLength ? text.right(maximumLength) : text;
    if (m_recentText == capped) {
        return;
    }

    m_recentText = capped;
    Q_EMIT recentTextChanged();
}

bool InputEngine::selectCandidate(int index)
{
    if (m_candidatesAreSuggestions) {
        if (index < 0 || index >= m_candidates.size()) {
            return false;
        }
        const QString word = m_candidates.at(index);
        const int replaceLength = m_suggestionReplaceLength;
        commit(word + u' ', -replaceLength, replaceLength);
        Q_EMIT suggestionAccepted(word);
        return true;
    }

    if (!m_textComposer) {
        return false;
    }

    return m_textComposer->selectCandidate(index);
}

void InputEngine::commit()
{
    if (!m_preeditText.isEmpty()) {
        const QString committed = m_preeditText;
        m_backend->commitText(committed);
        clearCompositionState(true);
        setRecentText(m_recentText + committed);
    }
}

void InputEngine::commit(const QString &text, int replaceFrom, int replaceLength)
{
    QString recentText = m_recentText;
    if (replaceLength != 0) {
        m_backend->deleteSurroundingText(replaceFrom, replaceLength);

        // Drop the replaced UTF-8 bytes from the end of the typed text
        int bytes = replaceLength;
        while (bytes > 0 && !recentText.isEmpty()) {
            bytes -= QStringView(recentText).right(1).toUtf8().size();
            recentText.chop(1);
        }
    }

    if (!text.isEmpty()) {
        m_backend->commitText(text);
    }

    clearCompositionState(true);
    setRecentText(recentText + text);
}

void InputEngine::clear()
{
    clearCompositionState(true);
}

bool InputEngine::handleKeyCommit(int key, const QString &text)
{
    switch (key) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Tab:
    case Qt::Key_Space:
        m_backend->sendKeyClick(key);
        trackRecentKey(key);
        return true;
    default:
        break;
    }

    if (text.isEmpty()) {
        return false;
    }

    commit(text);
    if (m_shiftActive && !m_capsLockActive) {
        setShiftActive(false);
    }
    return true;
}

void InputEngine::connectBackend()
{
    if (!m_backend->parent()) {
        m_backend->setParent(this);
    }

    connect(m_backend, &InputBackend::surroundingTextChanged, this, [this] {
        Q_EMIT surroundingTextChanged();
        Q_EMIT selectedTextChanged();
        Q_EMIT anchorPositionChanged();
        Q_EMIT cursorPositionChanged();
        updateAutomaticShift();
    });
    connect(m_backend, &InputBackend::inputMethodHintsChanged, this, [this] {
        Q_EMIT inputMethodHintsChanged();
        resetShiftState();
    });
    connect(m_backend, &InputBackend::pressedKeysChanged, this, [this] {
        Q_EMIT pressedKeysChanged();
        Q_EMIT uppercaseChanged();
    });
    connect(m_backend, &InputBackend::activeChanged, this, [this] {
        setRecentText(QString());
        Q_EMIT surroundingTextChanged();
        Q_EMIT selectedTextChanged();
        Q_EMIT anchorPositionChanged();
        Q_EMIT cursorPositionChanged();
        Q_EMIT inputMethodHintsChanged();
        resetShiftState();
    });
    connect(m_backend, &InputBackend::resetRequested, this, [this] {
        setRecentText(QString());
        if (m_textComposer) {
            m_textComposer->reset();
        } else {
            clear();
        }
        resetShiftState();
    });
}

void InputEngine::resetShiftState()
{
    if (!m_backend->isActive()) {
        setCapsLockActive(false);
        setShiftActive(false);
        return;
    }

    const auto hints = inputMethodHints();
    const bool preferUppercase =
        m_textComposer && m_textComposer->autoCapitalizationSupported() && hints.testAnyFlags(Qt::ImhPreferUppercase | Qt::ImhUppercaseOnly);
    setCapsLockActive(preferUppercase);
    setShiftActive(false);

    if (!preferUppercase) {
        updateAutomaticShift();
    }
}

void InputEngine::updateAutomaticShift()
{
    if (m_capsLockActive) {
        return;
    }

    if (!m_textComposer || !m_textComposer->autoCapitalizationSupported()) {
        setShiftActive(false);
        return;
    }

    const auto hints = inputMethodHints();
    const auto disablingHints = Qt::ImhNoAutoUppercase | Qt::ImhUppercaseOnly | Qt::ImhLowercaseOnly | Qt::ImhEmailCharactersOnly | Qt::ImhUrlCharactersOnly
        | Qt::ImhDialableCharactersOnly | Qt::ImhFormattedNumbersOnly | Qt::ImhDigitsOnly;
    if (!m_backend->isActive() || hints.testAnyFlags(disablingHints) || hints.testFlag(Qt::ImhPreferLowercase) || !m_preeditText.isEmpty()) {
        setShiftActive(false);
        return;
    }

    const QString textBeforeCursor = surroundingText().left(cursorPosition());
    setShiftActive(m_textComposer->shouldAutoCapitalize(textBeforeCursor));
}

void InputEngine::clearCompositionState(bool updateBackend)
{
    const bool hadPreedit = !m_preeditText.isEmpty();
    const bool hadCandidates = !m_candidates.isEmpty();

    if (updateBackend && hadPreedit) {
        m_backend->setPreeditText(QString());
    }

    if (hadPreedit) {
        m_preeditText.clear();
        Q_EMIT preeditTextChanged();
    }
    if (!m_preeditPrefix.isEmpty()) {
        m_preeditPrefix.clear();
        Q_EMIT preeditPrefixChanged();
    }
    m_candidatesAreSuggestions = false;
    if (hadCandidates) {
        m_candidates.clear();
        Q_EMIT candidatesChanged();
        Q_EMIT wordCandidateListVisibleHintChanged();
    }
}

bool InputEngine::sendDirectKeyClick(int key)
{
    return m_backend->sendKeyClick(key);
}

int InputEngine::utf16PositionFromUtf8Offset(uint32_t offset) const
{
    return QString::fromUtf8(surroundingText().toUtf8().first(offset)).size();
}
