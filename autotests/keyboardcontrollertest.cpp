/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "keyboardcontroller.h"
#include "inputbackend.h"
#include "inputengine.h"
#include "keyboardpackageresolver.h"

#include <QLoggingCategory>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(PlasmaKeyboard, "org.kde.plasma.keyboard")

class HintsInputBackend : public InputBackend
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
        return {};
    }
    uint32_t cursorPositionUtf8() const override
    {
        return 0;
    }
    uint32_t anchorPositionUtf8() const override
    {
        return 0;
    }
    void setPreeditText(const QString &) override
    {
    }
    void commitText(const QString &) override
    {
    }
    void deleteSurroundingText(int, int) override
    {
    }
    bool sendKeyClick(int) override
    {
        return true;
    }
    bool sendKeyPressed(int, bool) override
    {
        return true;
    }

    void setHints(Qt::InputMethodHints newHints)
    {
        hints = newHints;
        Q_EMIT inputMethodHintsChanged();
    }

    Qt::InputMethodHints hints = Qt::ImhNone;
};

class KeyboardControllerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layerOverridesLayoutType()
    {
        auto *backend = new HintsInputBackend;
        InputEngine engine(backend);
        KeyboardPackageResolver resolver;
        KeyboardController controller(&engine, &resolver);
        QSignalSpy layoutTypeSpy(&controller, &KeyboardController::layoutTypeChanged);
        QSignalSpy layerSpy(&controller, &KeyboardController::layerChanged);

        QVERIFY(controller.layer().isEmpty());
        QCOMPARE(controller.layoutType(), u"main"_s);

        controller.setLayer(u"editing"_s);

        QCOMPARE(controller.layer(), u"editing"_s);
        QCOMPARE(controller.layoutType(), u"editing"_s);
        QCOMPARE(layerSpy.count(), 1);
        QCOMPARE(layoutTypeSpy.count(), 1);
    }

    void layerWinsOverInputHints()
    {
        auto *backend = new HintsInputBackend;
        InputEngine engine(backend);
        KeyboardPackageResolver resolver;
        KeyboardController controller(&engine, &resolver);
        backend->setHints(Qt::ImhDigitsOnly);
        QCOMPARE(controller.layoutType(), u"digits"_s);

        // A layer is an explicit user choice, so it is shown even in a digits field
        controller.setLayer(u"terminal"_s);
        QCOMPARE(controller.layoutType(), u"terminal"_s);

        controller.setLayer(QString());
        QCOMPARE(controller.layoutType(), u"digits"_s);
    }

    void toggleLayerOpensAndCloses()
    {
        auto *backend = new HintsInputBackend;
        InputEngine engine(backend);
        KeyboardPackageResolver resolver;
        KeyboardController controller(&engine, &resolver);

        controller.toggleLayer(u"emoji"_s);
        QCOMPARE(controller.layer(), u"emoji"_s);

        // Toggling another layer switches to it
        controller.toggleLayer(u"editing"_s);
        QCOMPARE(controller.layer(), u"editing"_s);

        // Toggling the active layer goes back to the regular layout
        controller.toggleLayer(u"editing"_s);
        QVERIFY(controller.layer().isEmpty());
    }

    void symbolModeAndLayersAreExclusive()
    {
        auto *backend = new HintsInputBackend;
        InputEngine engine(backend);
        KeyboardPackageResolver resolver;
        KeyboardController controller(&engine, &resolver);

        controller.setLayer(u"emoji"_s);
        controller.setSymbolMode(true);
        QVERIFY(controller.layer().isEmpty());
        QCOMPARE(controller.layoutType(), u"symbols"_s);

        controller.setLayer(u"emoji"_s);
        QVERIFY(!controller.symbolMode());
        QCOMPARE(controller.layoutType(), u"emoji"_s);
    }
};

QTEST_GUILESS_MAIN(KeyboardControllerTest)

#include "keyboardcontrollertest.moc"
