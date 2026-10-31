/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

/**
 * A word list with usage frequencies, for word completion and gesture typing.
 *
 * Words the user types are learned and can be persisted to a file.
 */
class WordDictionary
{
public:
    struct Entry {
        QString word;
        int frequency = 0;
    };

    /**
     * Loads the bundled word list for a locale (e.g. "en_US"). Returns false if there is none.
     */
    bool loadLanguage(const QString &locale);

    /**
     * Replaces the base words with "<word> <frequency>" lines.
     */
    void loadData(QByteArrayView data);

    /**
     * Up to limit words starting with prefix, most frequent first. A word that
     * exactly matches the prefix comes first. The prefix's case is applied to the results.
     */
    QStringList completions(const QString &prefix, int limit) const;

    int frequency(const QString &word) const;
    bool contains(const QString &word) const;
    int size() const;

    /**
     * All words, sorted alphabetically (lowercase).
     */
    const QList<Entry> &entries() const;

    /**
     * Applies the case of typed text to a dictionary word.
     */
    static QString matchCase(const QString &word, const QString &typed);

    void setLearnedWordsFile(const QString &path);
    void learn(const QString &word);
    void saveLearnedWords() const;

private:
    void rebuild();

    QHash<QString, int> m_baseWords;
    QHash<QString, int> m_learnedWords;
    QList<Entry> m_entries;
    QString m_learnedWordsFile;
};
