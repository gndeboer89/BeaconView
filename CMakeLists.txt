cmake_minimum_required(VERSION 3.21)
project(BeaconView VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2)
qt_standard_project_setup()

qt_add_executable(beaconview WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/servicemodel.cpp src/servicemodel.h
    src/songlibrary.cpp  src/songlibrary.h
    src/presenter.cpp    src/presenter.h
)

qt_add_resources(beaconview "qml" PREFIX "/" FILES
    qml/Main.qml
    qml/SlideView.qml
    qml/AppButton.qml
    qml/Backgrounds.js
    qml/Theme.js
    qml/ServicePanel.qml
    qml/PreviewPanel.qml
    qml/SidePanel.qml
    qml/ScreensPage.qml
    qml/TextEditorDialog.qml
    qml/OutputWindow.qml
    qml/Placeholder.qml
)

set_target_properties(beaconview PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER nl.beaconview.app
    MACOSX_BUNDLE_BUNDLE_NAME BeaconView
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)
target_link_libraries(beaconview PRIVATE Qt6::Quick Qt6::QuickControls2)

include(GNUInstallDirs)
install(TARGETS beaconview BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
