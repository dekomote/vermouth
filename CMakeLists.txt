cmake_minimum_required(VERSION 3.20)
project(vermouth VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Gui Widgets QuickControls2)

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
)

target_compile_definitions(vermouth PRIVATE
    APP_VERSION_STRING="${PROJECT_VERSION}"
)
target_compile_definitions(vermouth PRIVATE
    APP_NAME_STRING="${PROJECT_NAME}"
)

install(TARGETS vermouth DESTINATION bin)
install(FILES assets/vermouth.desktop DESTINATION share/applications)
install(FILES assets/vermouth.svg DESTINATION share/icons/hicolor/scalable/apps)
