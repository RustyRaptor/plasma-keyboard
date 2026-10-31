/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include <memory>

class InputBackend;
class InputEngine;
class KeyboardController;
class KeyboardPackageResolver;
class SuggestionController;
class WordDictionary;

/**
 * Owns the input and layout services for a virtual keyboard instance.
 */
class VirtualKeyboardContext : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(InputEngine *inputEngine READ inputEngine CONSTANT)
    Q_PROPERTY(KeyboardController *keyboardController READ keyboardController CONSTANT)
    Q_PROPERTY(KeyboardPackageResolver *keyboardPackageResolver READ keyboardPackageResolver CONSTANT)

public:
    explicit VirtualKeyboardContext(InputBackend *inputBackend, QObject *parent = nullptr);

    InputEngine *inputEngine() const;
    KeyboardController *keyboardController() const;
    KeyboardPackageResolver *keyboardPackageResolver() const;
    SuggestionController *suggestionController() const;

    /**
     * Words of the current input language, shared by word suggestions and gesture typing.
     */
    std::shared_ptr<WordDictionary> wordDictionary() const;

Q_SIGNALS:
    void wordDictionaryChanged();

private:
    void loadWordDictionary();

    InputEngine *m_inputEngine = nullptr;
    KeyboardPackageResolver *m_keyboardPackageResolver = nullptr;
    KeyboardController *m_keyboardController = nullptr;
    SuggestionController *m_suggestionController = nullptr;
    std::shared_ptr<WordDictionary> m_wordDictionary;
    QString m_wordDictionaryLanguage;
};
