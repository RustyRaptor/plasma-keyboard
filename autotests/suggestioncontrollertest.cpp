/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "suggestioncontroller.h"
#include "basictextcomposer.h"
#include "inputbackend.h"
#include "inputengine.h"
#include "worddictionary.h"

#include <QLoggingCategory>
#include <QTest>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(PlasmaKeyboard, "org.kde.plasma.keyboard")

class TextInputBackend : public InputBackend
{
public:
    bool isActive() const override
    {
        return true;
    }
    Qt::InputMethodHints inputMethodHints() const override
    {
        return hints;
    }
    QString surroundingText() const override
    {
        return text;
    }
    uint32_t cursorPositionUtf8() const override
    {
        return text.toUtf8().size();
    }
    uint32_t anchorPositionUtf8() const override
    {
        return cursorPositionUtf8();
    }
    void setPreeditText(const QString &) override
    {
    }
    void commitText(const QString &committed) override
    {
        commits.append(committed);
    }
    void deleteSurroundingText(int, int length) override
    {
        deletes.append(length);
    }
    bool sendKeyClick(int) override
    {
        return true;
    }
    bool sendKeyPressed(int, bool) override
    {
        return true;
    }

    // Simulates the client reporting its text, with the cursor at the end
    void setText(const QString &newText)
    {
        text = newText;
        Q_EMIT surroundingTextChanged();
    }

    void setHints(Qt::InputMethodHints newHints)
    {
        hints = newHints;
        Q_EMIT inputMethodHintsChanged();
    }

    QString text;
    Qt::InputMethodHints hints = Qt::ImhNoAutoUppercase;
    QStringList commits;
    QList<int> deletes;
};

class SuggestionControllerTest : public QObject
{
    Q_OBJECT

private:
    std::shared_ptr<WordDictionary> sampleDictionary()
    {
        auto dictionary = std::make_shared<WordDictionary>();
        dictionary->loadData("help 80\nhello 50\nhelicopter 10\nworld 100\n");
        return dictionary;
    }

private Q_SLOTS:
    void suggestsCompletionsForCurrentWord()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());
        controller.setEnabled(true);

        backend->setText(u"I need hel"_s);

        QCOMPARE(engine.candidates(), QStringList({u"help"_s, u"hello"_s, u"helicopter"_s}));

        // Accepting replaces the partial word
        QVERIFY(engine.selectCandidate(1));
        QCOMPARE(backend->deletes, QList<int>{3});
        QCOMPARE(backend->commits, QStringList{u"hello "_s});
    }

    void clearsSuggestionsBetweenWords()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());
        controller.setEnabled(true);

        backend->setText(u"hel"_s);
        QVERIFY(!engine.candidates().isEmpty());
        backend->setText(u"help "_s);
        QVERIFY(engine.candidates().isEmpty());
    }

    void staysQuietWhenDisabledOrSensitive()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());

        backend->setText(u"hel"_s);
        QVERIFY(engine.candidates().isEmpty());

        controller.setEnabled(true);
        QVERIFY(!engine.candidates().isEmpty());

        // Never look at passwords
        backend->setHints(Qt::ImhSensitiveData | Qt::ImhNoAutoUppercase);
        QVERIFY(engine.candidates().isEmpty());
        backend->setHints(Qt::ImhUrlCharactersOnly | Qt::ImhNoAutoUppercase);
        QVERIFY(engine.candidates().isEmpty());
    }

    void fallsBackToTypedText()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());
        controller.setEnabled(true);

        // The client doesn't report surrounding text, so the typed keys are used
        engine.sendTextComposerKey(Qt::Key_W, u"w"_s);
        engine.sendTextComposerKey(Qt::Key_O, u"o"_s);

        QCOMPARE(engine.candidates(), QStringList({u"world"_s}));
    }

    void mergesLanguageModelPrediction()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());
        controller.setEnabled(true);

        backend->setText(u"Please hel"_s);
        controller.applyPrediction(u"Please hel"_s, u"helicopter"_s);

        // The model's guess goes first, without duplicates
        QCOMPARE(engine.candidates(), QStringList({u"helicopter"_s, u"help"_s, u"hello"_s}));

        // Next word predictions show up even without a word being typed
        backend->setText(u"Hello "_s);
        controller.applyPrediction(u"Hello "_s, u"world"_s);
        QCOMPARE(engine.candidates(), QStringList({u"world"_s}));
    }

    void dropsPredictionsOutsideTheDictionary()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());
        controller.setEnabled(true);

        // Small models repeat letters of words they don't know
        backend->setText(u"Hi hih"_s);
        controller.applyPrediction(u"Hi hih"_s, u"hihhiiiiiiiii"_s);
        QCOMPARE(engine.candidates(), QStringList());

        // Words typed often enough count as known
        auto dictionary = sampleDictionary();
        dictionary->learn(u"Hihgi"_s);
        dictionary->learn(u"Hihgi"_s);
        controller.setDictionary(dictionary);
        backend->setText(u"Hi hih"_s);
        controller.applyPrediction(u"Hi hih"_s, u"hihgi"_s);
        QCOMPARE(engine.candidates(), QStringList({u"hihgi"_s}));
    }

    void ignoresStalePredictions()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        SuggestionController controller(&engine);
        controller.setDictionary(sampleDictionary());
        controller.setEnabled(true);

        backend->setText(u"Hello wo"_s);
        controller.applyPrediction(u"Hello w"_s, u"wonderful"_s);

        QCOMPARE(engine.candidates(), QStringList({u"world"_s}));
    }

    void learnsAcceptedAndTypedWords()
    {
        auto *backend = new TextInputBackend;
        InputEngine engine(backend);
        engine.setTextComposer(new BasicTextComposer);
        auto dictionary = sampleDictionary();
        SuggestionController controller(&engine);
        controller.setDictionary(dictionary);
        controller.setEnabled(true);

        // Finishing a word with a space learns it
        backend->setText(u"plasma "_s);
        backend->setText(u"plasma plasma "_s);
        QVERIFY(dictionary->contains(u"plasma"_s));

        // Nothing is learned from sensitive fields
        backend->setHints(Qt::ImhSensitiveData | Qt::ImhNoAutoUppercase);
        backend->setText(u"hunter "_s);
        backend->setText(u"hunter hunter "_s);
        QVERIFY(!dictionary->contains(u"hunter"_s));
    }
};

QTEST_GUILESS_MAIN(SuggestionControllerTest)

#include "suggestioncontrollertest.moc"
