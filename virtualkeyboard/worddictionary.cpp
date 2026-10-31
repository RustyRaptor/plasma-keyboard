/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "worddictionary.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>

#include <algorithm>

using namespace Qt::StringLiterals;

// How much one use of a learned word counts, relative to the corpus frequencies
static constexpr int LEARNED_WORD_WEIGHT = 1000;
// Unknown words need this many uses before being suggested, so a single typo isn't
static constexpr int MINIMUM_NEW_WORD_USES = 2;

bool WordDictionary::loadLanguage(const QString &locale)
{
    const QString language = locale.section(u'_', 0, 0).section(u'-', 0, 0).toLower();
    QFile file(u":/plasma-keyboard/words/%1_words.txt"_s.arg(language));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    loadData(file.readAll());
    return true;
}

void WordDictionary::loadData(QByteArrayView data)
{
    m_baseWords.clear();
    const QList<QByteArray> lines = data.toByteArray().split('\n');
    m_baseWords.reserve(lines.size());
    for (const QByteArray &line : lines) {
        const qsizetype separator = line.lastIndexOf(' ');
        if (separator <= 0) {
            continue;
        }
        const QString word = QString::fromUtf8(line.left(separator)).toLower();
        m_baseWords.insert(word, line.sliced(separator + 1).trimmed().toInt());
    }
    rebuild();
}

QStringList WordDictionary::completions(const QString &prefix, int limit) const
{
    if (prefix.isEmpty() || limit <= 0) {
        return {};
    }

    const QString lowerPrefix = prefix.toLower();
    auto it = std::lower_bound(m_entries.cbegin(), m_entries.cend(), lowerPrefix, [](const Entry &entry, const QString &value) {
        return entry.word < value;
    });

    QList<Entry> matches;
    const Entry *exactMatch = nullptr;
    for (; it != m_entries.cend() && it->word.startsWith(lowerPrefix); ++it) {
        if (it->word == lowerPrefix) {
            exactMatch = &*it;
        } else {
            matches.append(*it);
        }
    }

    const qsizetype count = qMin<qsizetype>(matches.size(), limit);
    std::partial_sort(matches.begin(), matches.begin() + count, matches.end(), [](const Entry &a, const Entry &b) {
        return a.frequency > b.frequency;
    });

    QStringList result;
    if (exactMatch) {
        result.append(matchCase(exactMatch->word, prefix));
    }
    for (qsizetype i = 0; i < count && result.size() < limit; ++i) {
        result.append(matchCase(matches.at(i).word, prefix));
    }
    return result;
}

int WordDictionary::frequency(const QString &word) const
{
    const QString lowerWord = word.toLower();
    return m_baseWords.value(lowerWord) + m_learnedWords.value(lowerWord) * LEARNED_WORD_WEIGHT;
}

bool WordDictionary::contains(const QString &word) const
{
    const QString lowerWord = word.toLower();
    return m_baseWords.contains(lowerWord) || m_learnedWords.value(lowerWord) >= MINIMUM_NEW_WORD_USES;
}

int WordDictionary::size() const
{
    return m_entries.size();
}

const QList<WordDictionary::Entry> &WordDictionary::entries() const
{
    return m_entries;
}

QString WordDictionary::matchCase(const QString &word, const QString &typed)
{
    if (typed.size() > 1 && typed == typed.toUpper() && typed != typed.toLower()) {
        return word.toUpper();
    }

    QString result = word;
    // The English pronoun "I" and its contractions are always capitalized
    const bool pronoun = word == u"i" || word.startsWith(u"i'"_s);
    if (!result.isEmpty() && (pronoun || (!typed.isEmpty() && typed.front().isUpper()))) {
        result.front() = result.front().toUpper();
    }
    return result;
}

void WordDictionary::setLearnedWordsFile(const QString &path)
{
    m_learnedWordsFile = path;
    m_learnedWords.clear();

    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QList<QByteArray> lines = file.readAll().split('\n');
        for (const QByteArray &line : lines) {
            const qsizetype separator = line.lastIndexOf(' ');
            if (separator > 0) {
                m_learnedWords.insert(QString::fromUtf8(line.left(separator)), line.sliced(separator + 1).toInt());
            }
        }
    }
    rebuild();
}

void WordDictionary::learn(const QString &word)
{
    static const QRegularExpression wordPattern(u"^\\p{L}+('\\p{L}+)?$"_s);
    if (word.size() < 2 || !wordPattern.match(word).hasMatch()) {
        return;
    }

    const QString lowerWord = word.toLower();
    const bool wasKnown = contains(lowerWord);
    ++m_learnedWords[lowerWord];
    if (!wasKnown) {
        if (contains(lowerWord)) {
            rebuild();
        }
        return;
    }

    auto it = std::lower_bound(m_entries.begin(), m_entries.end(), lowerWord, [](const Entry &entry, const QString &value) {
        return entry.word < value;
    });
    if (it != m_entries.end() && it->word == lowerWord) {
        it->frequency = frequency(lowerWord);
    }
}

void WordDictionary::saveLearnedWords() const
{
    if (m_learnedWordsFile.isEmpty()) {
        return;
    }

    QDir().mkpath(QFileInfo(m_learnedWordsFile).absolutePath());
    QSaveFile file(m_learnedWordsFile);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    for (auto it = m_learnedWords.cbegin(); it != m_learnedWords.cend(); ++it) {
        file.write(it.key().toUtf8() + ' ' + QByteArray::number(it.value()) + '\n');
    }
    file.commit();
}

void WordDictionary::rebuild()
{
    m_entries.clear();
    m_entries.reserve(m_baseWords.size() + m_learnedWords.size());
    for (auto it = m_baseWords.cbegin(); it != m_baseWords.cend(); ++it) {
        m_entries.append({it.key(), frequency(it.key())});
    }
    for (auto it = m_learnedWords.cbegin(); it != m_learnedWords.cend(); ++it) {
        if (!m_baseWords.contains(it.key()) && it.value() >= MINIMUM_NEW_WORD_USES) {
            m_entries.append({it.key(), frequency(it.key())});
        }
    }
    std::sort(m_entries.begin(), m_entries.end(), [](const Entry &a, const Entry &b) {
        return a.word < b.word;
    });
}
