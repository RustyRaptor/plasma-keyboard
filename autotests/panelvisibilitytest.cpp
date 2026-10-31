/*
    SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "panelvisibility.h"

#include <QSignalSpy>
#include <QTest>

class PanelVisibilityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void showsWithContext()
    {
        PanelVisibility visibility;
        QVERIFY(!visibility.visible());

        QSignalSpy spy(&visibility, &PanelVisibility::visibleChanged);
        visibility.setHasContext(true);

        QVERIFY(visibility.visible());
        QCOMPARE(spy.count(), 1);
    }

    void hidesWithoutDelayByDefault()
    {
        PanelVisibility visibility;
        visibility.setHasContext(true);

        visibility.setHasContext(false);

        QVERIFY(!visibility.visible());
    }

    void lingersWhenContextFlickers()
    {
        PanelVisibility visibility;
        visibility.setHideDelay(200);
        visibility.setHasContext(true);
        QSignalSpy spy(&visibility, &PanelVisibility::visibleChanged);

        // Web pages often drop and recreate the text input while the user is typing
        visibility.setHasContext(false);
        QVERIFY(visibility.visible());
        QTest::qWait(50);
        visibility.setHasContext(true);
        QTest::qWait(250);

        QVERIFY(visibility.visible());
        QCOMPARE(spy.count(), 0);
    }

    void hidesAfterDelayWhenContextStaysGone()
    {
        PanelVisibility visibility;
        visibility.setHideDelay(50);
        visibility.setHasContext(true);

        visibility.setHasContext(false);

        QVERIFY(visibility.visible());
        QTRY_VERIFY_WITH_TIMEOUT(!visibility.visible(), 1000);
    }

    void pinnedStaysVisibleWithoutContext()
    {
        PanelVisibility visibility;
        visibility.setHasContext(true);
        visibility.setPinned(true);

        visibility.setHasContext(false);
        QTest::qWait(20);

        QVERIFY(visibility.visible());
    }

    void unpinningWithoutContextHides()
    {
        PanelVisibility visibility;
        visibility.setHasContext(true);
        visibility.setPinned(true);
        visibility.setHasContext(false);

        visibility.setPinned(false);

        QVERIFY(!visibility.visible());
    }

    void pinningDoesNotShowByItself()
    {
        PanelVisibility visibility;

        // Pinning only keeps an already shown keyboard around
        visibility.setPinned(true);

        QVERIFY(!visibility.visible());
    }

    void userHideWinsOverPinAndContext()
    {
        PanelVisibility visibility;
        visibility.setHasContext(true);
        visibility.setPinned(true);

        visibility.hide();
        QVERIFY(!visibility.visible());

        // Losing and regaining a context shows the keyboard again
        visibility.setHasContext(false);
        QVERIFY(!visibility.visible());
        visibility.setHasContext(true);
        QVERIFY(visibility.visible());
    }

    void contextActivityClearsUserHide()
    {
        PanelVisibility visibility;
        visibility.setHasContext(true);
        visibility.hide();

        // Interacting with the focused text field again brings the keyboard back
        visibility.notifyContextActivity();

        QVERIFY(visibility.visible());
    }

    void contextActivityWithoutContextIsIgnored()
    {
        PanelVisibility visibility;

        visibility.notifyContextActivity();

        QVERIFY(!visibility.visible());
    }

    void exposesWhetherTypingNeedsFallback()
    {
        PanelVisibility visibility;
        visibility.setHideDelay(200);
        visibility.setHasContext(true);
        QVERIFY(!visibility.detached());

        // Visible without a text input context: keys have to go elsewhere
        visibility.setHasContext(false);
        QVERIFY(visibility.detached());
    }
};

QTEST_GUILESS_MAIN(PanelVisibilityTest)

#include "panelvisibilitytest.moc"
