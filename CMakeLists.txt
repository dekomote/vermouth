cmake_minimum_required(VERSION 3.20)
project(vermouth VERSION 1.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

set(APP_ID "com.dekomote.vermouth")
set(APP_DESCRIPTION "A no-frills Wine/Proton game launcher for Linux")
set(APP_LICENSE "MIT")
set(APP_HOMEPAGE "https://github.com/dekomote/vermouth")
set(APP_BUG_ADDRESS "https://github.com/dekomote/vermouth/issues")
set(APP_AUTHOR_NAME "Dejan Noveski")
set(APP_AUTHOR_EMAIL "deko@duck.com")
set(APP_AUTHOR_URL "https://github.com/dekomote")
set(APP_COPYRIGHT "(c) 2026")
string(TIMESTAMP APP_RELEASE_DATE "%Y-%m-%d")
set(APP_LONG_DESCRIPTION "Vermouth is a no-frills lightweight game and application launcher that lets you run Windows executables through Proton or Wine on KDE.")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Quick Gui Widgets QuickControls2 Network DBus)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Kirigami CoreAddons I18n QQC2DesktopStyle)

ecm_find_qmlmodule(org.kde.kirigami REQUIRED)

qt_add_executable(vermouth
    src/main.cpp
    src/appentry.cpp
    src/appmodel.cpp
    src/protonscanner.cpp
    src/launcher.cpp
    src/desktopfilewriter.cpp
    src/iconextractor.cpp
    src/settingsmanager.cpp
    src/protondownloader.cpp
    resources.qrc
)

qt_add_qml_module(vermouth
    URI Vermouth
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Main.qml
        qml/AppGridView.qml
        qml/AddAppDialog.qml
        qml/RunExeDialog.qml
        qml/SettingsDialog.qml
    NO_RESOURCE_TARGET_PATH
)

target_link_libraries(vermouth PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Gui
    Qt6::Widgets
    Qt6::QuickControls2
    Qt6::Network
    Qt6::DBus
    KF6::Kirigami
    KF6::CoreAddons
    KF6::I18n
)

target_compile_definitions(vermouth PRIVATE
    APP_VERSION_STRING="${PROJECT_VERSION}"
    APP_NAME_STRING="${PROJECT_NAME}"
    APP_DESCRIPTION="${APP_DESCRIPTION}"
    APP_LICENSE="${APP_LICENSE}"
    APP_HOMEPAGE="${APP_HOMEPAGE}"
    APP_BUG_ADDRESS="${APP_BUG_ADDRESS}"
    APP_AUTHOR_NAME="${APP_AUTHOR_NAME}"
    APP_AUTHOR_EMAIL="${APP_AUTHOR_EMAIL}"
    APP_AUTHOR_URL="${APP_AUTHOR_URL}"
    APP_COPYRIGHT="${APP_COPYRIGHT}"
    APP_ID="${APP_ID}"
)

install(TARGETS vermouth ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES assets/${APP_ID}.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES assets/${APP_ID}.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
configure_file(assets/${APP_ID}.metainfo.xml.in ${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.metainfo.xml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_CONTACT "${APP_AUTHOR_EMAIL}")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${APP_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_LICENSE "${APP_LICENSE}")
set(CPACK_RPM_PACKAGE_URL "${APP_HOMEPAGE}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base, qt6-declarative, qt6-quickcontrols2, kirigami2")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtdeclarative, qt6-qtquickcontrols2, kf6-kirigami")
set(CPACK_RPM_PACKAGE_RECOMMENDS "icoutils")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_GENERATOR "DEB;RPM")

include(CPack)
