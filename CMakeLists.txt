cmake_minimum_required(VERSION 3.20)
project(vermouth VERSION 0.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMFindQmlModule)
include(ECMQmlModule)


find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Quick Gui Widgets QuickControls2)
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
    NO_RESOURCE_TARGET_PATH
)

target_link_libraries(vermouth PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Gui
    Qt6::Widgets
    Qt6::QuickControls2
    KF6::Kirigami
    KF6::CoreAddons
    KF6::I18n
)

target_compile_definitions(vermouth PRIVATE
    APP_VERSION_STRING="${PROJECT_VERSION}"
    APP_NAME_STRING="${PROJECT_NAME}"
)

install(TARGETS vermouth ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES assets/vermouth.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES assets/vermouth.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)
install(FILES assets/com.dekomote.vermouth.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})


set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "deko@duck.com")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_DESCRIPTION "")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A no-frills Wine/Proton game launcher for Linux.")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_URL "https://github.com/dekomote/vermouth/")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base, qt6-declarative, qt6-quickcontrols2, kirigami2")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtdeclarative, qt6-qtquickcontrols2, kf6-kirigami")
set(CPACK_RPM_PACKAGE_RECOMMENDS "icoutils")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_GENERATOR "DEB;RPM")

include(CPack)
