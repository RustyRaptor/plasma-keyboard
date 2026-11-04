/*
    SPDX-FileCopyrightText: 2024 Aleix Pol i Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "buildinfo.h"
#include "config-plasma-keyboard.h"
#include "configfilewatcher.h"
#include "inputpanelintegration.h"
#include "logging.h"
#include "plasmakeyboardsettings.h"
#include "portalcolorscheme.h"

#include <KAboutData>
#include <KConfigWatcher>
#include <KCrash>
#include <KLocalizedQmlContext>
#include <KLocalizedString>
#include <KSandbox>

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QWindow>
#include <qpa/qwindowsysteminterface.h>

// signal handler for SIGINT & SIGTERM
#ifdef Q_OS_UNIX
#include <KSignalHandler>
#include <signal.h>
#include <unistd.h>
#endif

int main(int argc, char **argv)
{
    // Gesture typing needs every touch point of a swipe, not one per frame
    if (!qEnvironmentVariableIsSet("QML_NO_TOUCH_COMPRESSION")) {
        qputenv("QML_NO_TOUCH_COMPRESSION", "1");
    }

    // KWin closes the connection when it switches to another input method. Plasma sessions turn on Qt's
    // reconnect, which would keep this keyboard running as an ordinary client beside the new one.
    qunsetenv("QT_WAYLAND_RECONNECT");

    // KWin hands the input method it starts a connection in WAYLAND_SOCKET, which connecting consumes
    const bool startedByCompositor = qEnvironmentVariableIsSet("WAYLAND_SOCKET");

    QGuiApplication application(argc, argv);

    KLocalizedString::setApplicationDomain("plasma-keyboard");

    KAboutData aboutData(QStringLiteral("plasma-keyboard"),
                         i18n("Plasma Keyboard"),
                         BuildInfo::summary(),
                         i18n("An on-screen keyboard for Plasma"),
                         KAboutLicense::GPL,
                         i18n("Copyright 2024, Aleix Pol Gonzalez"));

    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), i18n("Author"), QStringLiteral("aleixpol@kde.org"));
    aboutData.setOrganizationDomain("kde.org");
    aboutData.setDesktopFileName(QStringLiteral("org.kde.plasma.keyboard"));
    application.setWindowIcon(QIcon::fromTheme(QStringLiteral("input-keyboard-virtual")));
    aboutData.setProgramLogo(application.windowIcon());

    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

    {
        QCommandLineParser parser;
        aboutData.setupCommandLine(&parser);
        parser.process(application);
        aboutData.processCommandLine(&parser);
    }

    // KWin lets any client bind the input method protocol, so a copy started any other way, such as by name
    // from Plasma, would compete with the keyboard KWin started.
    if (!startedByCompositor) {
        qCCritical(PlasmaKeyboard)
            << "Cannot run plasma-keyboard standalone. You can enable it in Plasma's System Settings app, on the “Virtual Keyboard” page.";
        return 1;
    }

    if (!PLASMA_KEYBOARD_SOUNDS_ENABLED) {
        PlasmaKeyboardSettings::self()->setSoundEnabled(false);
    }

    if (!PLASMA_KEYBOARD_VIBRATION_ENABLED) {
        PlasmaKeyboardSettings::self()->setVibrationEnabled(false);
    }

    // Listen to config updates from kcm, and reparse
    const auto reloadSettings = [] {
        PlasmaKeyboardSettings::self()->sharedConfig()->reparseConfiguration();
        PlasmaKeyboardSettings::self()->load();
    };
    auto watcher = KConfigWatcher::create(PlasmaKeyboardSettings::self()->sharedConfig());
    QObject::connect(watcher.get(), &KConfigWatcher::configChanged, &application, reloadSettings);

    // Change notifications are D-Bus signals that don't reach a Flatpak sandbox, so watch the file too
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u'/' + PlasmaKeyboardSettings::self()->config()->name();
    auto *fileWatcher = new ConfigFileWatcher(configPath, &application);
    QObject::connect(fileWatcher, &ConfigFileWatcher::changed, &application, reloadSettings);

    // The sandbox's copy of the desktop's colors goes stale when the color scheme changes
    if (KSandbox::isFlatpak()) {
        auto *colors = new PortalColorScheme(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + u"/plasma-keyboard/colors", &application);
        colors->start();
    }

    QQmlApplicationEngine view;
    KLocalization::setupLocalizedContext(&view);

    QObject::connect(&view, &QQmlApplicationEngine::objectCreated, &application, [](QObject *object) {
        auto window = qobject_cast<QWindow *>(object);
        const bool initSuccessful = initInputPanelIntegration(window, InputPanelRole::Keyboard);

        if (!initSuccessful) {
            qCCritical(PlasmaKeyboard)
                << "Cannot run plasma-keyboard standalone. You can enable it in Plasma's System Settings app, on the “Virtual Keyboard” page.";
            exit(1);
        }

        // Rely on InputMethodConnection to show the window once there is input context
    });
    view.load(QUrl(QStringLiteral("qrc:/qt/qml/org/kde/plasma/keyboard/main.qml")));

#ifdef Q_OS_UNIX
    /**
     * Set up signal handler for SIGINT and SIGTERM
     */
    KSignalHandler::self()->watchSignal(SIGINT);
    KSignalHandler::self()->watchSignal(SIGTERM);
    QObject::connect(KSignalHandler::self(), &KSignalHandler::signalReceived, &application, [](int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            qCDebug(PlasmaKeyboard) << "Received signal" << signal << ", exiting now.";
            QCoreApplication::quit();
        }
    });
#endif

    qCDebug(PlasmaKeyboard) << "Starting Plasma Keyboard" << BuildInfo::summary();

    return application.exec();
}
