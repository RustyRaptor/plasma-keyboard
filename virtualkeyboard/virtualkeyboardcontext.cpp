/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "virtualkeyboardcontext.h"

#include "inputbackend.h"
#include "inputengine.h"
#include "keyboardcontroller.h"
#include "keyboardpackageresolver.h"
#include "suggestioncontroller.h"
#include "worddictionary.h"

#include <QStandardPaths>

using namespace Qt::StringLiterals;

VirtualKeyboardContext::VirtualKeyboardContext(InputBackend *inputBackend, QObject *parent)
    : QObject(parent)
    , m_inputEngine(new InputEngine(inputBackend, this))
    , m_keyboardPackageResolver(new KeyboardPackageResolver(this))
    , m_keyboardController(new KeyboardController(m_inputEngine, m_keyboardPackageResolver, this))
    , m_suggestionController(new SuggestionController(m_inputEngine, this))
{
    connect(m_inputEngine, &InputEngine::localeChanged, this, &VirtualKeyboardContext::loadWordDictionary);
    loadWordDictionary();
}

InputEngine *VirtualKeyboardContext::inputEngine() const
{
    return m_inputEngine;
}

KeyboardController *VirtualKeyboardContext::keyboardController() const
{
    return m_keyboardController;
}

KeyboardPackageResolver *VirtualKeyboardContext::keyboardPackageResolver() const
{
    return m_keyboardPackageResolver;
}

SuggestionController *VirtualKeyboardContext::suggestionController() const
{
    return m_suggestionController;
}

std::shared_ptr<WordDictionary> VirtualKeyboardContext::wordDictionary() const
{
    return m_wordDictionary;
}

void VirtualKeyboardContext::loadWordDictionary()
{
    const QString language = m_inputEngine->locale().section(u'_', 0, 0);
    if (language == m_wordDictionaryLanguage && m_wordDictionary) {
        return;
    }

    m_wordDictionaryLanguage = language;
    auto dictionary = std::make_shared<WordDictionary>();
    if (dictionary->loadLanguage(language)) {
        const QString dataDirectory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        dictionary->setLearnedWordsFile(dataDirectory + u"/plasma-keyboard/learned-words-%1.txt"_s.arg(language));
        m_wordDictionary = dictionary;
    } else {
        m_wordDictionary.reset();
    }
    m_suggestionController->setDictionary(m_wordDictionary);
    Q_EMIT wordDictionaryChanged();
}
