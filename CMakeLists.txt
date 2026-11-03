# SPDX-FileCopyrightText: 2024 Aleix Pol <aleixpol@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

set(PROJECT_VERSION "6.8.80")

project(plasma-keyboard VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_DEP_VERSION "6.7.90")
set(QT_MIN_VERSION "6.11.0")
set(KF6_MIN_VERSION "6.30.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs) # Should be included first so its variables are available to other modules.

include(ECMDeprecationSettings)
include(ECMInstallIcons)
include(ECMQmlModule)
include(ECMQtDeclareLoggingCategory)
include(ECMSetupVersion)

include(FeatureSummary)

include(KDEClangFormat)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEGitCommitHooks)

option(PLASMA_KEYBOARD_SOUNDS_ENABLED "Whether to enable sound effect support, requires GPLv3 compliance" ON)
option(PLASMA_KEYBOARD_VIBRATION_ENABLED "Whether to enable haptic feedback via vibrations, requires GPLv3 compliance" ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Network
    Qml
    Quick
    WaylandClient
)

# For voice input, and sound effects
find_package(Qt6Multimedia ${QT_MIN_VERSION} REQUIRED)

find_package(Qt6GuiPrivate ${QT_MIN_VERSION} REQUIRED NO_MODULE)
find_package(Qt6WaylandClientPrivate ${QT_MIN_VERSION} REQUIRED NO_MODULE)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    I18n
    KCMUtils
    Package
    Config
    Crash
)

find_package(PlasmaQuick ${PROJECT_DEP_VERSION} REQUIRED)

ecm_set_disabled_deprecation_versions(QT 6.11.0
    KF 6.29.0
)

ecm_setup_version(${PROJECT_VERSION} VARIABLE_PREFIX PLASMA_KEYBOARD VERSION_HEADER plasma_keyboard_version.h)

# The git commit of the source, refreshed by every build, so the keyboard can say which build it is
add_custom_target(plasma_keyboard_build_id ALL
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DOUTPUT=${CMAKE_BINARY_DIR}/plasma_keyboard_build_id.h
            -P ${CMAKE_SOURCE_DIR}/cmake/PlasmaKeyboardBuildId.cmake
    BYPRODUCTS ${CMAKE_BINARY_DIR}/plasma_keyboard_build_id.h
    COMMENT "Recording the git commit of this build"
    VERBATIM
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(XKBCommon REQUIRED IMPORTED_TARGET xkbcommon)

find_package(Wayland 1.2 REQUIRED COMPONENTS Client)
find_package(QtWaylandScanner REQUIRED ${QT_MIN_VERSION})
find_package(WaylandProtocols 1.19 REQUIRED)
find_package(PlasmaWaylandProtocols REQUIRED)

add_subdirectory(virtualkeyboard)
add_subdirectory(layouts)
add_subdirectory(kcm)
add_subdirectory(src)
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

ki18n_install(po)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
