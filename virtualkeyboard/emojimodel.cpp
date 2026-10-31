/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "emojimodel.h"

#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>

#include <algorithm>
#include <limits>

using namespace Qt::StringLiterals;

static QString emojiFontFamily()
{
    const QStringList families = QFontDatabase::families();
    if (families.contains(u"Noto Color Emoji"_s)) {
        return u"Noto Color Emoji"_s;
    }
    for (const QString &family : families) {
        if (family.contains(u"Emoji"_s, Qt::CaseInsensitive)) {
            return family;
        }
    }
    return {};
}

EmojiModel::EmojiModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Parsing the bundled data takes a while, so share it between instances
    static const QList<Group> bundledGroups = [] {
        EmojiModel model(nullptr, {});
        QFile file(u":/plasma-keyboard/emoji/emoji-test.txt"_s);
        if (file.open(QIODevice::ReadOnly)) {
            model.loadData(file.readAll());
        }
        if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
            const QFont emojiFont(emojiFontFamily());
            const QFontMetrics metrics(emojiFont);
            model.removeUnsupported([&metrics](char32_t codePoint) {
                return metrics.inFontUcs4(codePoint);
            });
        }
        return model.m_groups;
    }();
    m_groups = bundledGroups;
}

EmojiModel::EmojiModel(QObject *parent, NoBundledData)
    : QAbstractListModel(parent)
{
}

int EmojiModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    if (m_currentGroup == RecentGroup) {
        return m_recent.size();
    }
    if (m_currentGroup < 0 || m_currentGroup >= m_groups.size()) {
        return 0;
    }
    return m_groups.at(m_currentGroup).emoji.size();
}

QVariant EmojiModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    Emoji emoji;
    if (m_currentGroup == RecentGroup) {
        emoji = findEmoji(m_recent.at(index.row()));
    } else {
        emoji = m_groups.at(m_currentGroup).emoji.at(index.row());
    }

    switch (role) {
    case Qt::DisplayRole:
    case EmojiRole:
        return emoji.emoji;
    case NameRole:
        return emoji.name;
    case VariantsRole:
        return emoji.variants.isEmpty() ? QStringList() : QStringList{emoji.emoji} + emoji.variants;
    default:
        return {};
    }
}

QHash<int, QByteArray> EmojiModel::roleNames() const
{
    return {
        {EmojiRole, "emoji"},
        {NameRole, "name"},
        {VariantsRole, "variants"},
    };
}

void EmojiModel::removeUnsupported(const std::function<bool(char32_t)> &hasGlyph)
{
    // Find the oldest version with an emoji the font can't show
    qreal firstUnsupported = std::numeric_limits<qreal>::max();
    for (const auto &group : std::as_const(m_groups)) {
        for (const auto &emoji : group.emoji) {
            QList<uint> codePoints = emoji.emoji.toUcs4();
            codePoints.removeAll(0xFE0F);
            if (codePoints.size() == 1 && !hasGlyph(codePoints.constFirst())) {
                firstUnsupported = qMin(firstUnsupported, emoji.version);
            }
        }
    }
    if (firstUnsupported == std::numeric_limits<qreal>::max()) {
        return;
    }

    beginResetModel();
    for (auto &group : m_groups) {
        group.emoji.removeIf([firstUnsupported](const Emoji &emoji) {
            return emoji.version >= firstUnsupported;
        });
    }
    endResetModel();
}

void EmojiModel::loadData(QByteArrayView data)
{
    beginResetModel();
    m_groups.clear();

    bool skipGroup = false;
    const QList<QByteArray> lines = data.toByteArray().split('\n');
    for (const QByteArray &line : lines) {
        if (line.startsWith("# group: ")) {
            const QString name = QString::fromUtf8(line.sliced(9)).trimmed();
            // The component group only holds building blocks such as bare skin tones
            skipGroup = name == u"Component";
            if (!skipGroup) {
                m_groups.append({name, {}});
            }
            continue;
        }
        if (skipGroup || m_groups.isEmpty() || line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const qsizetype statusStart = line.indexOf(';');
        const qsizetype commentStart = line.indexOf('#');
        if (statusStart < 0 || commentStart < statusStart || line.sliced(statusStart + 1, commentStart - statusStart - 1).trimmed() != "fully-qualified") {
            continue;
        }

        // The comment is "<emoji> E<version> <name>"
        const QString comment = QString::fromUtf8(line.sliced(commentStart + 1)).trimmed();
        const qsizetype emojiEnd = comment.indexOf(u' ');
        const qsizetype versionEnd = comment.indexOf(u' ', emojiEnd + 1);
        if (emojiEnd < 0 || versionEnd < 0) {
            continue;
        }
        const QString emoji = comment.left(emojiEnd);
        const QString name = comment.sliced(versionEnd + 1);
        // The version is written as "E<major>.<minor>"
        const qreal version = comment.sliced(emojiEnd + 2, versionEnd - emojiEnd - 2).toDouble();

        auto &groupEmoji = m_groups.last().emoji;
        const qsizetype qualifierStart = name.indexOf(u": "_s);
        if (qualifierStart > 0 && name.sliced(qualifierStart).contains(u"skin tone"_s)) {
            const QString baseName = name.left(qualifierStart);
            auto base = std::find_if(groupEmoji.rbegin(), groupEmoji.rend(), [&baseName](const Emoji &candidate) {
                return candidate.name == baseName;
            });
            if (base != groupEmoji.rend()) {
                base->variants.append(emoji);
                continue;
            }
        }
        groupEmoji.append({emoji, name, {}, version});
    }

    endResetModel();
    Q_EMIT groupsChanged();
}

QStringList EmojiModel::groups() const
{
    QStringList names;
    names.reserve(m_groups.size());
    for (const auto &group : m_groups) {
        names.append(group.name);
    }
    return names;
}

QStringList EmojiModel::groupIcons() const
{
    QStringList icons;
    icons.reserve(m_groups.size());
    for (const auto &group : m_groups) {
        icons.append(group.emoji.isEmpty() ? QString() : group.emoji.constFirst().emoji);
    }
    return icons;
}

int EmojiModel::currentGroup() const
{
    return m_currentGroup;
}

void EmojiModel::setCurrentGroup(int group)
{
    if (m_currentGroup == group) {
        return;
    }

    beginResetModel();
    m_currentGroup = group;
    endResetModel();
    Q_EMIT currentGroupChanged();
}

QStringList EmojiModel::recent() const
{
    return m_recent;
}

void EmojiModel::setRecent(const QStringList &recent)
{
    const QStringList capped = recent.mid(0, m_maximumRecent);
    if (m_recent == capped) {
        return;
    }

    const bool showingRecent = m_currentGroup == RecentGroup;
    if (showingRecent) {
        beginResetModel();
    }
    m_recent = capped;
    if (showingRecent) {
        endResetModel();
    }
    Q_EMIT recentChanged();
}

int EmojiModel::maximumRecent() const
{
    return m_maximumRecent;
}

void EmojiModel::setMaximumRecent(int maximum)
{
    if (m_maximumRecent == maximum) {
        return;
    }

    m_maximumRecent = maximum;
    Q_EMIT maximumRecentChanged();
    setRecent(m_recent);
}

void EmojiModel::noteUsed(const QString &emoji)
{
    QStringList recent = m_recent;
    recent.removeAll(emoji);
    recent.prepend(emoji);
    setRecent(recent);
}

EmojiModel::Emoji EmojiModel::findEmoji(const QString &emoji) const
{
    for (const auto &group : m_groups) {
        for (const auto &candidate : group.emoji) {
            if (candidate.emoji == emoji || candidate.variants.contains(emoji)) {
                return candidate.emoji == emoji ? candidate : Emoji{emoji, candidate.name, {}};
            }
        }
    }
    return {emoji, {}, {}};
}

#include "moc_emojimodel.cpp"
