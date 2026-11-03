/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "customlayout.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace
{
struct NamedKey {
    QString type;
    QString properties;
};

const QHash<QString, NamedKey> &namedKeys()
{
    static const QHash<QString, NamedKey> keys = {
        {u"escape"_s, {u"KeysymKey"_s, u"key: Qt.Key_Escape; displayText: \"Esc\""_s}},
        {u"tab"_s, {u"KeysymKey"_s, u"key: Qt.Key_Tab; displayText: \"Tab\""_s}},
        {u"backspace"_s, {u"BackspaceKey"_s, {}}},
        {u"enter"_s, {u"EnterKey"_s, {}}},
        {u"shift"_s, {u"ShiftKey"_s, u"sendDirectKeyEvents: true"_s}},
        {u"space"_s, {u"SpaceKey"_s, {}}},
        {u"symbols"_s, {u"SymbolModeKey"_s, {}}},
        {u"emoji"_s, {u"LayerKey"_s, u"targetLayer: \"emoji\"; iconName: \"smiley-symbolic\""_s}},
        {u"ctrl"_s, {u"KeysymKey"_s, u"key: Qt.Key_Control; displayText: \"Ctrl\"; sticky: true"_s}},
        {u"alt"_s, {u"KeysymKey"_s, u"key: Qt.Key_Alt; displayText: \"Alt\"; sticky: true"_s}},
        {u"meta"_s, {u"KeysymKey"_s, u"key: Qt.Key_Meta; iconName: \"start-here-symbolic\"; displayText: \"\"; sticky: true"_s}},
        {u"left"_s, {u"KeysymKey"_s, u"key: Qt.Key_Left; iconName: \"go-previous-symbolic\"; displayText: \"\"; repeat: true"_s}},
        {u"right"_s, {u"KeysymKey"_s, u"key: Qt.Key_Right; iconName: \"go-next-symbolic\"; displayText: \"\"; repeat: true"_s}},
        {u"up"_s, {u"KeysymKey"_s, u"key: Qt.Key_Up; iconName: \"go-up-symbolic\"; displayText: \"\"; repeat: true"_s}},
        {u"down"_s, {u"KeysymKey"_s, u"key: Qt.Key_Down; iconName: \"go-down-symbolic\"; displayText: \"\"; repeat: true"_s}},
        {u"language"_s, {u"ChangeLanguageKey"_s, {}}},
        {u"hide"_s, {u"HideKeyboardKey"_s, {}}},
        {u"gap"_s, {u"FillerKey"_s, {}}},
    };
    return keys;
}

// Accented alternatives of the desktop layout, without its number row digits
const QHash<QString, QString> &defaultAlternatives()
{
    static const QHash<QString, QString> alternatives = {
        {u"a"_s, u"aäåãâàá"_s}, {u"c"_s, u"çcċčć"_s},      {u"d"_s, u"dđď"_s},        {u"e"_s, u"êeëèé"_s},  {u"g"_s, u"ġgģĝğ"_s},
        {u"i"_s, u"îïīĩiìí"_s}, {u"l"_s, u"ĺŀłļľl"_s},     {u"n"_s, u"ņńnň"_s},       {u"o"_s, u"œøõôöòóo"_s}, {u"r"_s, u"ŕrř"_s},
        {u"s"_s, u"šsşś"_s},    {u"t"_s, u"ţtŧť"_s},       {u"u"_s, u"űūũûüuùú"_s},   {u"y"_s, u"ÿyýŷ"_s},   {u"z"_s, u"zžż"_s},
        {u"'"_s, u"\""_s},      {u","_s, u";"_s},          {u"."_s, u":"_s},          {u"?"_s, u"!"_s},
    };
    return alternatives;
}

QString quoted(const QString &text)
{
    QString escaped = text;
    escaped.replace(u'\\', u"\\\\"_s);
    escaped.replace(u'"', u"\\\""_s);
    escaped.replace(u'\n', u"\\n"_s);
    return u'"' + escaped + u'"';
}

QString weightProperty(double weight)
{
    return u"weight: root.defaultKeyWeight * "_s + QString::number(weight);
}

QString keyLine(const QString &type, QStringList properties)
{
    properties.removeAll(QString());
    if (properties.isEmpty()) {
        return type + u" { }"_s;
    }
    return type + u" { "_s + properties.join(u"; "_s) + u" }"_s;
}

const QString arrowCluster = uR"(KeysymKey { key: Qt.Key_Left; iconName: "go-previous-symbolic"; displayText: ""; repeat: true }
        KeyboardColumn {
            weight: root.defaultKeyWeight
            KeysymKey { implicitHeight: -1; key: Qt.Key_Up; iconName: "go-up-symbolic"; displayText: ""; repeat: true }
            KeysymKey { implicitHeight: -1; key: Qt.Key_Down; iconName: "go-down-symbolic"; displayText: ""; repeat: true }
        }
        KeysymKey { key: Qt.Key_Right; iconName: "go-next-symbolic"; displayText: ""; repeat: true })"_s;
}

CustomLayout::CustomLayout(QObject *parent)
    : QObject(parent)
    , m_cacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + u"/custom-layouts"_s)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &CustomLayout::reload);
    // Editors often replace the file instead of writing to it
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &CustomLayout::reload);
}

CustomLayout::~CustomLayout()
{
    if (m_url.isLocalFile()) {
        QFile::remove(m_url.toLocalFile());
    }
}

QStringList CustomLayout::keyNames()
{
    QStringList names = namedKeys().keys();
    names.append(u"arrows"_s);
    names.sort();
    return names;
}

CustomLayout::Result CustomLayout::generate(QByteArrayView json)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toByteArray(), &parseError);
    if (document.isNull()) {
        return {{}, u"Not valid JSON: %1"_s.arg(parseError.errorString())};
    }
    const QJsonValue rowsValue = document.object().value(u"rows"_s);
    if (!rowsValue.isArray() || rowsValue.toArray().isEmpty()) {
        return {{}, u"Expected \"rows\": a list of rows of keys"_s};
    }

    const QJsonArray rows = rowsValue.toArray();
    QString body;
    double widestRow = 0;
    for (qsizetype rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        const QString rowName = u"row %1"_s.arg(rowIndex + 1);
        const QJsonArray entries = rows.at(rowIndex).toArray();
        if (entries.isEmpty()) {
            return {{}, rowName + u" is empty"_s};
        }

        QStringList lines;
        double rowWeight = 0;
        for (const QJsonValue &entry : entries) {
            const QJsonObject object = entry.toObject();
            const QString name = entry.isString() ? entry.toString() : object.value(u"key"_s).toString();
            double weight = 1;
            if (object.contains(u"weight"_s)) {
                weight = object.value(u"weight"_s).toDouble();
                if (weight <= 0) {
                    return {{}, u"The weight of a key in %1 must be a positive number"_s.arg(rowName)};
                }
            }
            const QString weightText = weight == 1 ? QString() : weightProperty(weight);

            if (object.contains(u"chars"_s)) {
                const QJsonObject alternatives = object.value(u"alternatives"_s).toObject();
                const QList<uint> characters = object.value(u"chars"_s).toString().toUcs4();
                for (const uint character : characters) {
                    const QString text = QString::fromUcs4(reinterpret_cast<const char32_t *>(&character), 1);
                    const QString alternativeKeys = alternatives.contains(text) ? alternatives.value(text).toString() : defaultAlternatives().value(text);
                    const uint key = QChar::toUpper(character);
                    lines.append(keyLine(u"Key"_s,
                                         {u"key: %1"_s.arg(key),
                                          u"text: "_s + quoted(text),
                                          alternativeKeys.isEmpty() ? QString() : u"alternativeKeys: "_s + quoted(alternativeKeys),
                                          weightText}));
                    rowWeight += weight;
                }
                continue;
            }

            if (name == u"arrows") {
                lines.append(arrowCluster);
                rowWeight += 3;
                continue;
            }

            const auto namedKey = namedKeys().constFind(name);
            if (namedKey == namedKeys().constEnd()) {
                return {{}, u"Unknown key \"%1\" in %2. Known keys: %3"_s.arg(name, rowName, keyNames().join(u", "_s))};
            }
            lines.append(keyLine(namedKey->type, {namedKey->properties, weightText}));
            rowWeight += weight;
        }

        widestRow = std::max(widestRow, rowWeight);
        body += u"\n    KeyboardRow {\n        "_s + lines.join(u"\n        "_s) + u"\n    }\n"_s;
    }

    QString qml = uR"(// Generated from a custom layout file, edit that file instead
import QtQuick
import QtQuick.Layouts

import org.kde.plasma.keyboard.virtualkeyboard
import org.kde.plasma.keyboard.virtualkeyboard.components

KeyboardLayout {
    id: root

    // Keep normal keys square
    maxWidthToHeightRatio: %1
)"_s.arg(QString::number(widestRow / rows.size()));
    qml += body + u"}\n"_s;
    return {qml, {}};
}

QString CustomLayout::file() const
{
    return m_file;
}

void CustomLayout::setFile(const QString &file)
{
    if (m_file == file) {
        return;
    }

    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
    }
    m_file = file;
    if (!m_file.isEmpty()) {
        m_watcher.addPath(QFileInfo(m_file).absolutePath());
    }
    Q_EMIT fileChanged();
    reload();
}

QUrl CustomLayout::url() const
{
    return m_url;
}

QString CustomLayout::errorString() const
{
    return m_errorString;
}

void CustomLayout::setCacheDirectory(const QString &directory)
{
    m_cacheDirectory = directory;
}

void CustomLayout::reload()
{
    if (m_file.isEmpty()) {
        setResult({}, {});
        return;
    }

    // The file is watched again after an editor replaced it
    if (QFileInfo::exists(m_file) && !m_watcher.files().contains(m_file)) {
        m_watcher.addPath(m_file);
    }

    QFile file(m_file);
    if (!file.open(QIODevice::ReadOnly)) {
        setResult({}, u"Can't read %1: %2"_s.arg(m_file, file.errorString()));
        return;
    }

    const Result result = generate(file.readAll());
    if (!result.error.isEmpty()) {
        setResult({}, result.error);
        return;
    }

    // A new name for each content, since QML caches components by URL
    const QByteArray hash = QCryptographicHash::hash(result.qml.toUtf8(), QCryptographicHash::Sha1).toHex().left(12);
    const QString path = m_cacheDirectory + u"/layout-"_s + QString::fromLatin1(hash) + u".qml"_s;
    if (QUrl::fromLocalFile(path) == m_url) {
        setResult(m_url, {});
        return;
    }

    QDir().mkpath(m_cacheDirectory);
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(result.qml.toUtf8()) < 0 || !output.commit()) {
        setResult({}, u"Can't write %1: %2"_s.arg(path, output.errorString()));
        return;
    }
    setResult(QUrl::fromLocalFile(path), {});
}

void CustomLayout::setResult(const QUrl &url, const QString &error)
{
    if (m_url != url) {
        if (m_url.isLocalFile()) {
            QFile::remove(m_url.toLocalFile());
        }
        m_url = url;
        Q_EMIT urlChanged();
    }
    if (m_errorString != error) {
        m_errorString = error;
        Q_EMIT errorStringChanged();
    }
}

#include "moc_customlayout.cpp"
