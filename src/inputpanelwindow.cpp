/*
    SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "inputpanelwindow.h"

#include "inputpanelintegration.h"
#include "panelgeometry.h"
#include "plasmakeyboardsettings.h"

#include <KSandbox>
#include <QDesktopServices>
#include <QProcess>
#include <qnamespace.h>

InputPanelWindow::InputPanelWindow(QWindow *parent)
    : QQuickWindow{parent}
{
    setFlag(Qt::FramelessWindowHint);

    connect(this, &QWindow::widthChanged, this, &InputPanelWindow::updateMask);
    connect(this, &QWindow::heightChanged, this, &InputPanelWindow::updateMask);
}

QRect InputPanelWindow::interactiveRegion() const
{
    return m_interactiveRegion;
}

void InputPanelWindow::setInteractiveRegion(QRect interactiveRegion)
{
    if (interactiveRegion == m_interactiveRegion) {
        return;
    }
    m_interactiveRegion = interactiveRegion;
    Q_EMIT interactiveRegionChanged();
    updateMask();
}

void InputPanelWindow::updateMask()
{
    // Set only a part of the window to be interactive
    setMask(PanelGeometry::inputRegion(m_interactiveRegion, size()));
}

void InputPanelWindow::showSettings()
{
    if (KSandbox::isInside()) {
        QProcess::startDetached(QStringLiteral("kcmshell6"), {QStringLiteral("kcm_plasmakeyboard")});
    } else {
        QDesktopServices::openUrl(QUrl(QStringLiteral("systemsettings:kcm_plasmakeyboard")));
    }
}

void InputPanelWindow::resetPanelGeometry()
{
    auto *settings = PlasmaKeyboardSettings::self();
    const auto keys = {"panelFloating", "panelPositionX", "panelPositionY", "panelHeightScale", "panelWidthFraction"};
    for (const char *key : keys) {
        if (auto *item = settings->findItem(QString::fromLatin1(key))) {
            item->setDefault();
        }
    }
    settings->save();
}

bool InputPanelWindow::initInputPanel(InputPanelRole::Role role)
{
    return initInputPanelIntegration(this, role);
}

#include "moc_inputpanelwindow.cpp"
