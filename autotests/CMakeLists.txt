# SPDX-FileCopyrightText: 2026 Aleix Pol <aleixpol@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test WaylandCompositor)
include(ECMAddTests)

ecm_add_test(inputenginetest.cpp
    TEST_NAME inputenginetest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Gui
        Qt::Qml
        Qt::Test
)

file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mockinputmethodcompositor_config.h
    CONTENT "#pragma once\n#define PLASMA_KEYBOARD_BINARY_PATH \"$<TARGET_FILE:plasma-keyboard>\"\n"
)

ecm_add_test(mockinputmethodcompositor.cpp LINK_LIBRARIES
    KF6::ConfigCore
    PkgConfig::XKBCommon
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::Test
    Qt::WaylandCompositor
    Wayland::Server
)

qt6_generate_wayland_protocol_server_sources(mockinputmethodcompositor
    FILES
        ${WaylandProtocols_DATADIR}/unstable/input-method/input-method-unstable-v1.xml
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/fake-input.xml
        ${Wayland_DATADIR}/wayland.xml
)

ecm_add_test(steamdeckhapticstest.cpp ${CMAKE_SOURCE_DIR}/src/steamdeckhaptics.cpp
    TEST_NAME steamdeckhapticstest
    LINK_LIBRARIES
        Qt::Core
        Qt::Test
)
target_include_directories(steamdeckhapticstest PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(panelvisibilitytest.cpp ${CMAKE_SOURCE_DIR}/src/panelvisibility.cpp
    TEST_NAME panelvisibilitytest
    LINK_LIBRARIES
        Qt::Core
        Qt::Test
)
target_include_directories(panelvisibilitytest PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(panelgeometrytest.cpp ${CMAKE_SOURCE_DIR}/src/panelgeometry.cpp
    TEST_NAME panelgeometrytest
    LINK_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)
target_include_directories(panelgeometrytest PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(fakeinputtexttest.cpp ${CMAKE_SOURCE_DIR}/src/fakeinputtext.cpp
    TEST_NAME fakeinputtexttest
    LINK_LIBRARIES
        Qt::Core
        Qt::Test
)
target_include_directories(fakeinputtexttest PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(keyboardcontrollertest.cpp
    TEST_NAME keyboardcontrollertest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Gui
        Qt::Qml
        Qt::Test
)

ecm_add_test(cursordragtrackertest.cpp
    TEST_NAME cursordragtrackertest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Qml
        Qt::Test
)

ecm_add_test(emojimodeltest.cpp
    TEST_NAME emojimodeltest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Qml
        Qt::Test
)

ecm_add_test(worddictionarytest.cpp
    TEST_NAME worddictionarytest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Test
)

ecm_add_test(llmcompletionclienttest.cpp
    TEST_NAME llmcompletionclienttest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Network
        Qt::Test
)

ecm_add_test(suggestioncontrollertest.cpp
    TEST_NAME suggestioncontrollertest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Qml
        Qt::Test
)

ecm_add_test(gesturedecodertest.cpp
    TEST_NAME gesturedecodertest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Test
)

ecm_add_test(transcriptionclienttest.cpp
    TEST_NAME transcriptionclienttest
    LINK_LIBRARIES
        plasma-keyboard-virtualkeyboard
        Qt::Network
        Qt::Test
)

if(TARGET Qt::Multimedia)
    ecm_add_test(audioconversiontest.cpp ${CMAKE_SOURCE_DIR}/src/audioconversion.cpp
        TEST_NAME audioconversiontest
        LINK_LIBRARIES
            Qt::Multimedia
            Qt::Test
    )
    target_include_directories(audioconversiontest PRIVATE ${CMAKE_SOURCE_DIR}/src)
endif()

if(TARGET Qt::Multimedia)
    ecm_add_test(voiceinputtest.cpp ${CMAKE_SOURCE_DIR}/src/voiceinput.cpp ${CMAKE_SOURCE_DIR}/src/audioconversion.cpp
        TEST_NAME voiceinputtest
        LINK_LIBRARIES
            plasma-keyboard-virtualkeyboard
            Qt::Multimedia
            Qt::Qml
            Qt::Test
    )
    target_include_directories(voiceinputtest PRIVATE ${CMAKE_SOURCE_DIR}/src)
endif()

ecm_add_test(configfilewatchertest.cpp ${CMAKE_SOURCE_DIR}/src/configfilewatcher.cpp
    TEST_NAME configfilewatchertest
    LINK_LIBRARIES
        Qt::Core
        Qt::Test
)
target_include_directories(configfilewatchertest PRIVATE ${CMAKE_SOURCE_DIR}/src)

ecm_add_test(emittedkeystest.cpp ${CMAKE_SOURCE_DIR}/src/emittedkeys.cpp
    TEST_NAME emittedkeystest
    LINK_LIBRARIES
        Qt::Core
        Qt::Test
)
target_include_directories(emittedkeystest PRIVATE ${CMAKE_SOURCE_DIR}/src)
