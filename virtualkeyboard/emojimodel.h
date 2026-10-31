/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QAbstractListModel>
#include <QStringList>

#include <functional>
#include <qqmlintegration.h>

/**
 * The emoji of one group (or the recently used ones), for the emoji layer.
 *
 * Data is read from Unicode's emoji-test.txt format. Skin tone variants are
 * folded into their base emoji.
 */
class EmojiModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)
    Q_PROPERTY(QStringList groupIcons READ groupIcons NOTIFY groupsChanged)
    Q_PROPERTY(int currentGroup READ currentGroup WRITE setCurrentGroup NOTIFY currentGroupChanged)
    Q_PROPERTY(QStringList recent READ recent WRITE setRecent NOTIFY recentChanged)
    Q_PROPERTY(int maximumRecent READ maximumRecent WRITE setMaximumRecent NOTIFY maximumRecentChanged)

public:
    enum Role {
        EmojiRole = Qt::UserRole + 1,
        NameRole,
        VariantsRole,
    };
    Q_ENUM(Role)

    enum SpecialGroup {
        // currentGroup value that shows the recently used emoji
        RecentGroup = -1,
    };
    Q_ENUM(SpecialGroup)

    explicit EmojiModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * Replaces the emoji data with data in emoji-test.txt format.
     */
    void loadData(QByteArrayView data);

    /**
     * Removes emoji from Unicode versions that the font doesn't fully support.
     *
     * Support is judged by whether every single code point emoji of a version has a glyph,
     * since sequences can't be checked reliably.
     */
    void removeUnsupported(const std::function<bool(char32_t)> &hasGlyph);

    QStringList groups() const;
    QStringList groupIcons() const;

    int currentGroup() const;
    void setCurrentGroup(int group);

    QStringList recent() const;
    void setRecent(const QStringList &recent);
    int maximumRecent() const;
    void setMaximumRecent(int maximum);

    /**
     * Moves an emoji to the front of the recently used ones.
     */
    Q_INVOKABLE void noteUsed(const QString &emoji);

Q_SIGNALS:
    void groupsChanged();
    void currentGroupChanged();
    void recentChanged();
    void maximumRecentChanged();

private:
    struct Emoji {
        QString emoji;
        QString name;
        QStringList variants;
        qreal version = 0;
    };
    struct Group {
        QString name;
        QList<Emoji> emoji;
    };

    struct NoBundledData {
    };
    EmojiModel(QObject *parent, NoBundledData);

    Emoji findEmoji(const QString &emoji) const;

    QList<Group> m_groups;
    int m_currentGroup = 0;
    QStringList m_recent;
    int m_maximumRecent = 32;
};
