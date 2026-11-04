# SPDX-FileCopyrightText: 2024 Aleix Pol <aleixpol@kde.org>
# SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
# SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>
#
# SPDX-License-Identifier: BSD-2-Clause

configure_file(config-plasma-keyboard.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma-keyboard.h)

add_executable(plasma-keyboard)

if (Qt6_VERSION VERSION_GREATER_EQUAL "6.8.0")
    set(private_code_option PRIVATE_CODE)
endif()

ecm_add_qtwayland_client_protocol(plasma-keyboard PROTOCOL ${WaylandProtocols_DATADIR}/unstable/input-method/input-method-unstable-v1.xml BASENAME input-method-unstable-v1)
ecm_add_qtwayland_client_protocol(plasma-keyboard PROTOCOL ${WaylandProtocols_DATADIR}/unstable/text-input/text-input-unstable-v1.xml BASENAME text-input-unstable-v1)
ecm_add_qtwayland_client_protocol(plasma-keyboard PROTOCOL "${Wayland_DATADIR}/wayland.xml" BASENAME wayland)
qt6_generate_wayland_protocol_client_sources(plasma-keyboard FILES ${PLASMA_WAYLAND_PROTOCOLS_DIR}/fake-input.xml ${private_code_option})

target_sources(plasma-keyboard PRIVATE
    audioconversion.cpp
    audioconversion.h
    buildinfo.cpp
    buildinfo.h
    configfilewatcher.cpp
    configfilewatcher.h
    emittedkeys.cpp
    emittedkeys.h
    fakeinputtext.cpp
    fakeinputtext.h
    feedback.cpp
    feedback.h
    keycodes.h
    kwinfakeinput.cpp
    kwinfakeinput.h
    main.cpp
    inputpanelintegration.cpp
    inputpanelintegration.h
    inputpanelrole.h
    inputmethodconnection.cpp
    inputmethodconnection.h
    inputmethodhints.cpp
    inputmethodhints.h
    inputmethod.cpp
    inputmethod_p.h
    inputpanelwindow.cpp
    inputpanelwindow.h
    inputplugin.cpp
    inputplugin.h
    panelgeometry.cpp
    panelgeometry.h
    panelvisibility.cpp
    panelvisibility.h
    portalcolorscheme.cpp
    portalcolorscheme.h
    vkbdwaylandinputbackend.cpp
    vkbdwaylandinputbackend.h
    voiceinput.cpp
    voiceinput.h
    qwaylandinputpanelshellintegration.cpp
    qwaylandinputpanelshellintegration_p.h
    qwaylandinputpanelsurface.cpp
    qwaylandinputpanelsurface_p.h
    overlay/overlaycontroller.cpp
    overlay/overlaycontroller.h
    overlay/candidatemodel.cpp
    overlay/candidatemodel.h
    overlay/overlaytrigger.cpp
    overlay/overlaytrigger.h
    overlay/longpresstrigger.cpp
    overlay/longpresstrigger.h
    overlay/diacriticsdataloader.cpp
    overlay/diacriticsdataloader.h
    overlay/diacritics.qrc
    overlay/prefixquerytrigger.cpp
    overlay/prefixquerytrigger.h
    overlay/textexpansiontrigger.cpp
    overlay/textexpansiontrigger.h
)

if(PLASMA_KEYBOARD_VIBRATION_ENABLED)
    set_source_files_properties(dbus/org.sigxcpu.Feedback.Haptic.xml PROPERTIES INCLUDE vibrationevent.h)
    qt_add_dbus_interfaces(dbusinterface_SRCS
        dbus/org.sigxcpu.Feedback.Haptic.xml)

    target_sources(plasma-keyboard PRIVATE
        steamdeckhaptics.cpp
        steamdeckhaptics.h
        vibrationevent.h
        ${dbusinterface_SRCS}
    )
endif()

if(PLASMA_KEYBOARD_SOUNDS_ENABLED)
    target_sources(plasma-keyboard PRIVATE
        soundresources.qrc
    )
endif()

ecm_qt_declare_logging_category(plasma-keyboard
    HEADER logging.h
    IDENTIFIER "PlasmaKeyboard"
    CATEGORY_NAME "org.kde.plasma.keyboard"
    DESCRIPTION "Plasma Keyboard"
    DEFAULT_SEVERITY Warning
)

ecm_add_qml_module(plasma-keyboard
    URI "org.kde.plasma.keyboard"
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES
    QtQuick
)

ecm_target_qml_sources(plasma-keyboard SOURCES
    qml/main.qml
    qml/KeyboardPanel.qml
    qml/DiacriticsOverlay.qml
    qml/OverlayWindow.qml
    qml/AdjustOverlay.qml
    qml/PanelDragHandler.qml
    qml/TypingPreview.qml
    qml/KeyboardToolbar.qml
    qml/ToolbarButton.qml
    qml/VoiceInputButton.qml
)
ecm_finalize_qml_module(plasma-keyboard)
add_dependencies(plasma-keyboard plasma_keyboard_build_id)

kconfig_add_kcfg_files(plasma-keyboard GENERATE_MOC plasmakeyboardsettings.kcfgc)

target_include_directories(plasma-keyboard PRIVATE
    ${CMAKE_BINARY_DIR}
    # Add directories to include path for QML type registration
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/overlay
)

target_link_libraries(plasma-keyboard
    PRIVATE
    KF6::ColorScheme
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Crash
    PkgConfig::XKBCommon
    Plasma::PlasmaQuick
    Qt::DBus
    Qt::Gui
    Qt::GuiPrivate
    Qt::WaylandClient
    Qt::WaylandClientPrivate
    Wayland::Client
    plasma-keyboard-virtualkeyboard
    plasma-keyboard-virtualkeyboardplugin
)

qt6_import_qml_plugins(plasma-keyboard)

target_link_libraries(plasma-keyboard PRIVATE Qt::Multimedia)

install(TARGETS plasma-keyboard ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.plasma.keyboard.desktop DESTINATION ${KDE_INSTALL_APPDIR})

install(DIRECTORY overlay/diacritics DESTINATION ${CMAKE_INSTALL_PREFIX}/share/plasma/keyboard)

install(FILES plasmakeyboardsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.plasma.keyboard.metainfo.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)
