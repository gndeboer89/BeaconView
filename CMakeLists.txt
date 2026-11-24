cmake_minimum_required(VERSION 3.21)
project(BeaconView VERSION 0.1.1.0 LANGUAGES CXX)
if(WIN32)
    enable_language(RC)   # for the application icon (assets/beaconview.rc)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Svg Pdf)
qt_standard_project_setup()

qt_add_executable(beaconview WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/servicemodel.cpp src/servicemodel.h
    src/songlibrary.cpp  src/songlibrary.h
    src/songimporter.cpp src/songimporter.h
    src/extrascreens.cpp src/extrascreens.h
    src/presenter.cpp    src/presenter.h
    src/bible.cpp        src/bible.h
    src/templatelibrary.cpp src/templatelibrary.h
    src/overlaycontroller.cpp src/overlaycontroller.h
    src/branding.cpp src/branding.h
    src/slideimporter.cpp src/slideimporter.h
    src/videohub.h
    src/videothumbnails.cpp src/videothumbnails.h
)
qt_add_resources(beaconview "bible_data" PREFIX "/" FILES data/bible_svv.json)
qt_add_resources(beaconview "assets" PREFIX "/" FILES assets/logo.png assets/icon/icon.png)

# Application icon: .rc on Windows, .icns in the app bundle on macOS
if(WIN32)
    target_sources(beaconview PRIVATE assets/beaconview.rc)
endif()
if(APPLE)
    set(BV_ICNS ${CMAKE_CURRENT_SOURCE_DIR}/assets/icon/beaconview.icns)
    target_sources(beaconview PRIVATE ${BV_ICNS})
    set_source_files_properties(${BV_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(beaconview PROPERTIES MACOSX_BUNDLE_ICON_FILE beaconview.icns)
endif()

qt_add_resources(beaconview "qml" PREFIX "/" FILES
    qml/Main.qml
    qml/SlideView.qml
    qml/AppButton.qml
    qml/Backgrounds.js
    qml/Theme.js
    qml/ServicePanel.qml
    qml/SlideEditor.qml
    qml/VideoEngine.qml
    qml/VideoOptionsDialog.qml
    qml/ImportDialog.qml
    qml/MediaPage.qml
    qml/PreviewPanel.qml
    qml/ControlPanel.qml
    qml/TemplatesPage.qml
    qml/OverlayLayer.qml
    qml/SettingsPage.qml
    qml/OverlayPanel.qml
    qml/PromptDialog.qml
    qml/ColorField.qml
    qml/SidePanel.qml
    qml/ScreensPage.qml
    qml/StageView.qml
    qml/SubtitleView.qml
    qml/ExtraWindow.qml
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
target_link_libraries(beaconview PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Svg Qt6::Pdf)

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
    qt_add_executable(tst_overlays tests/tst_overlays.cpp src/overlaycontroller.cpp src/overlaycontroller.h)
    target_include_directories(tst_overlays PRIVATE src)
    target_link_libraries(tst_overlays PRIVATE Qt6::Test)
    add_test(NAME overlays COMMAND tst_overlays)
    qt_add_executable(tst_branding tests/tst_branding.cpp src/branding.cpp src/branding.h)
    target_include_directories(tst_branding PRIVATE src)
    target_link_libraries(tst_branding PRIVATE Qt6::Test Qt6::Gui)
    add_test(NAME branding COMMAND tst_branding)
    qt_add_executable(tst_service tests/tst_service.cpp src/servicemodel.cpp src/servicemodel.h)
    target_include_directories(tst_service PRIVATE src)
    target_link_libraries(tst_service PRIVATE Qt6::Test)
    add_test(NAME service COMMAND tst_service)
    qt_add_executable(tst_presenter tests/tst_presenter.cpp
        src/presenter.cpp src/servicemodel.cpp src/songlibrary.cpp src/songimporter.cpp src/extrascreens.cpp src/bible.cpp
        src/templatelibrary.cpp src/overlaycontroller.cpp src/branding.cpp src/slideimporter.cpp)
    qt_add_resources(tst_presenter "bible_data" PREFIX "/" FILES data/bible_svv.json)
    target_include_directories(tst_presenter PRIVATE src)
    target_link_libraries(tst_presenter PRIVATE Qt6::Test Qt6::Gui Qt6::Pdf)
    add_test(NAME presenter COMMAND tst_presenter)
    qt_add_executable(tst_songimport tests/tst_songimport.cpp src/songlibrary.cpp src/songlibrary.h src/songimporter.cpp src/songimporter.h)
    target_include_directories(tst_songimport PRIVATE src)
    target_link_libraries(tst_songimport PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME songimport COMMAND tst_songimport)
    qt_add_executable(tst_extrascreens tests/tst_extrascreens.cpp src/extrascreens.cpp src/extrascreens.h)
    target_include_directories(tst_extrascreens PRIVATE src)
    target_link_libraries(tst_extrascreens PRIVATE Qt6::Test Qt6::Core)
    add_test(NAME extrascreens COMMAND tst_extrascreens)
    qt_add_executable(tst_importer tests/tst_importer.cpp src/slideimporter.cpp src/slideimporter.h)
    target_include_directories(tst_importer PRIVATE src)
    target_compile_definitions(tst_importer PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    target_link_libraries(tst_importer PRIVATE Qt6::Test Qt6::Gui Qt6::Pdf)
    add_test(NAME importer COMMAND tst_importer)
    qt_add_executable(tst_thumbnails tests/tst_thumbnails.cpp src/videothumbnails.cpp src/videothumbnails.h)
    target_include_directories(tst_thumbnails PRIVATE src)
    target_compile_definitions(tst_thumbnails PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data")
    target_link_libraries(tst_thumbnails PRIVATE Qt6::Test Qt6::Gui Qt6::Multimedia)
    add_test(NAME thumbnails COMMAND tst_thumbnails)
    qt_add_executable(tst_assets tests/tst_assets.cpp)
    qt_add_resources(tst_assets "assets" PREFIX "/" FILES assets/logo.png assets/icon/icon.png)
    target_link_libraries(tst_assets PRIVATE Qt6::Test Qt6::Gui)
    add_test(NAME assets COMMAND tst_assets)
endif()
