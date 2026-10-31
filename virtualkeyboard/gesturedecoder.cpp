/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "gesturedecoder.h"

#include "worddictionary.h"

#include <QLineF>

#include <algorithm>
#include <cmath>

namespace
{
// Number of points both paths are resampled to before comparing them
constexpr int SAMPLE_COUNT = 48;
// How far, in key widths, the swipe may start or end from a word's first or last key
constexpr qreal END_TOLERANCE = 1.1;
// Swipes shorter than this, in key widths, are taps
constexpr qreal MINIMUM_LENGTH = 0.8;
// Spread of the location and shape distances in the likelihood
constexpr qreal LOCATION_SIGMA = 0.35;
constexpr qreal SHAPE_SIGMA = 0.12;
// Spread of how far, in key widths, the swipe starts and ends from the word's first and last keys
constexpr qreal ENDPOINT_SIGMA = 0.45;
// Spread of the log ratio between the swipe's and the word's path lengths
constexpr qreal LENGTH_SIGMA = 0.3;
// Weight of the word frequency against the path likelihood
constexpr qreal FREQUENCY_WEIGHT = 0.4;

qreal gaussian(qreal value, qreal sigma)
{
    return -(value * value) / (2 * sigma * sigma);
}

qreal pathLength(const QList<QPointF> &path)
{
    qreal length = 0;
    for (qsizetype i = 1; i < path.size(); ++i) {
        length += QLineF(path.at(i - 1), path.at(i)).length();
    }
    return length;
}

QList<QPointF> resample(const QList<QPointF> &path)
{
    QList<QPointF> samples;
    samples.reserve(SAMPLE_COUNT);
    const qreal length = pathLength(path);
    if (path.isEmpty()) {
        return samples;
    }
    if (length <= 0) {
        samples.fill(path.constFirst(), SAMPLE_COUNT);
        return samples;
    }

    const qreal interval = length / (SAMPLE_COUNT - 1);
    samples.append(path.constFirst());
    qreal carried = 0;
    for (qsizetype i = 1; i < path.size() && samples.size() < SAMPLE_COUNT; ++i) {
        QPointF from = path.at(i - 1);
        const QPointF to = path.at(i);
        qreal segment = QLineF(from, to).length();
        while (carried + segment >= interval && samples.size() < SAMPLE_COUNT) {
            const qreal t = (interval - carried) / segment;
            from = from + (to - from) * t;
            samples.append(from);
            segment = QLineF(from, to).length();
            carried = 0;
        }
        carried += segment;
    }
    while (samples.size() < SAMPLE_COUNT) {
        samples.append(path.constLast());
    }
    return samples;
}

// Moves the path's centroid to the origin and scales its larger side to 1
QList<QPointF> normalizedShape(const QList<QPointF> &samples)
{
    QPointF centroid;
    qreal minX = samples.constFirst().x(), maxX = minX, minY = samples.constFirst().y(), maxY = minY;
    for (const QPointF &point : samples) {
        centroid += point;
        minX = qMin(minX, point.x());
        maxX = qMax(maxX, point.x());
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }
    centroid /= samples.size();
    const qreal scale = qMax(1.0, qMax(maxX - minX, maxY - minY));

    QList<QPointF> shape;
    shape.reserve(samples.size());
    for (const QPointF &point : samples) {
        shape.append((point - centroid) / scale);
    }
    return shape;
}

qreal meanDistance(const QList<QPointF> &a, const QList<QPointF> &b)
{
    qreal total = 0;
    for (qsizetype i = 0; i < a.size(); ++i) {
        total += QLineF(a.at(i), b.at(i)).length();
    }
    return total / a.size();
}
}

void GestureDecoder::setKeyCenters(const QHash<QChar, QPointF> &centers, qreal keyWidth)
{
    m_keyCenters = centers;
    m_keyWidth = keyWidth;
}

QStringList GestureDecoder::decode(const QList<QPointF> &path, const WordDictionary &dictionary, int limit) const
{
    if (m_keyCenters.isEmpty() || m_keyWidth <= 0 || path.size() < 2 || pathLength(path) < MINIMUM_LENGTH * m_keyWidth) {
        return {};
    }

    const QList<QPointF> samples = resample(path);
    const QList<QPointF> shape = normalizedShape(samples);
    const qreal endTolerance = END_TOLERANCE * m_keyWidth;
    const qreal length = pathLength(path);

    struct Candidate {
        QString word;
        qreal score;
    };
    QList<Candidate> candidates;

    // Only words starting and ending near the swipe's ends are worth comparing
    const auto nearEnd = [this, endTolerance](QChar letter, const QPointF &point) {
        const auto it = m_keyCenters.constFind(letter);
        return it != m_keyCenters.constEnd() && QLineF(*it, point).length() <= endTolerance;
    };

    QList<QPointF> keys;
    for (const auto &entry : dictionary.entries()) {
        if (entry.word.size() < 2 || !nearEnd(entry.word.front(), samples.constFirst()) || !nearEnd(entry.word.back(), samples.constLast())) {
            continue;
        }

        // Consecutive repeated letters are a single key on the path
        keys.clear();
        bool typeable = true;
        for (const QChar letter : entry.word) {
            const auto it = m_keyCenters.constFind(letter);
            if (it == m_keyCenters.constEnd()) {
                typeable = false;
                break;
            }
            if (keys.isEmpty() || keys.constLast() != *it) {
                keys.append(*it);
            }
        }
        if (!typeable || keys.size() < 2) {
            continue;
        }

        const QList<QPointF> template_ = resample(keys);
        const qreal location = meanDistance(samples, template_) / m_keyWidth;
        const qreal shapeDistance = meanDistance(shape, normalizedShape(template_));
        const qreal startDistance = QLineF(keys.constFirst(), samples.constFirst()).length() / m_keyWidth;
        const qreal endDistance = QLineF(keys.constLast(), samples.constLast()).length() / m_keyWidth;
        const qreal lengthRatio = std::log(length / qMax(1.0, pathLength(keys)));
        const qreal score = gaussian(location, LOCATION_SIGMA) + gaussian(shapeDistance, SHAPE_SIGMA) + gaussian(startDistance, ENDPOINT_SIGMA)
            + gaussian(endDistance, ENDPOINT_SIGMA) + gaussian(lengthRatio, LENGTH_SIGMA) + FREQUENCY_WEIGHT * std::log(entry.frequency + 1.0);
        candidates.append({entry.word, score});
    }

    const qsizetype count = qMin<qsizetype>(limit, candidates.size());
    std::partial_sort(candidates.begin(), candidates.begin() + count, candidates.end(), [](const Candidate &a, const Candidate &b) {
        return a.score > b.score;
    });

    QStringList words;
    for (qsizetype i = 0; i < count; ++i) {
        words.append(WordDictionary::matchCase(candidates.at(i).word, QString()));
    }
    return words;
}
