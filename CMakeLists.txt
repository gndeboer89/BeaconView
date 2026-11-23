cmake_minimum_required(VERSION 3.21)
project(BeaconView VERSION 0.0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Multimedia)
qt_standard_project_setup()

qt_add_executable(beaconview WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/servicemodel.cpp src/servicemodel.h
    src/songlibrary.cpp  src/songlibrary.h
    src/presenter.cpp    src/presenter.h
    src/bible.cpp        src/bible.h
    src/templatelibrary.cpp src/templatelibrary.h
)
qt_add_resources(beaconview "bible_data" PREFIX "/" FILES data/bible_svv.json)

qt_add_resources(beaconview "qml" PREFIX "/" FILES
    qml/Main.qml
    qml/SlideView.qml
    qml/AppButton.qml
    qml/Backgrounds.js
    qml/Theme.js
    qml/ServicePanel.qml
    qml/PreviewPanel.qml
    qml/ControlPanel.qml
    qml/TemplatesPage.qml
    qml/ColorField.qml
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
target_compile_definitions(beaconview PRIVATE APP_VERSION="${PROJECT_VERSION}")
target_link_libraries(beaconview PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia)

include(GNUInstallDirs)
install(TARGETS beaconview BUNDLE DESTINATION . RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Unit tests: cmake -DBUILD_TESTING=ON ... && ctest --test-dir build --output-on-failure
option(BUILD_TESTING "Build unit tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_bible tests/tst_bible.cpp src/bible.cpp src/bible.h)
    qt_add_resources(tst_bible "bible_data" PREFIX "/" FILES data/bible_svv.json)
    target_include_directories(tst_bible PRIVATE src)
    target_link_libraries(tst_bible PRIVATE Qt6::Test)
    add_test(NAME bible COMMAND tst_bible)
    qt_add_executable(tst_templates tests/tst_templates.cpp src/templatelibrary.cpp src/templatelibrary.h)
    target_include_directories(tst_templates PRIVATE src)
    target_link_libraries(tst_templates PRIVATE Qt6::Test)
    add_test(NAME templates COMMAND tst_templates)
endif()
