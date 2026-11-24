import QtQuick
import QtQml
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

ApplicationWindow {
    id: win
    width: 1440; height: 860; minimumWidth: 1320; minimumHeight: 760
    visible: true
    title: "BeaconView"
    color: T.bg
    // Dark palette so menus, dialogs and combo boxes match the UI
    palette {
        window: T.panel; windowText: T.text; base: T.card; text: T.text
        button: T.card; buttonText: T.text; highlight: T.accent; highlightedText: "white"
        mid: T.border; dark: T.border; light: T.cardHover; placeholderText: T.muted; toolTipBase: T.card; toolTipText: T.text
    }
    property int page: 0
    onClosing: Qt.quit()

    VideoEngine { id: videoEngine }
    ImportDialog {}
    OutputWindow {}
    Instantiator { model: presenter.extraScreens; delegate: ExtraWindow {} }   // stage and subtitle screens
    Component.onCompleted: {   // developer aid: --tab N --query "Joh 3:16"
        win.page = devPage
        if (devSong >= 0) { win.page = 1; songsPage.openSong(devSong === 99 ? -1 : devSong) }   // developer aid: --edit-song N (99 = a new song)
        if (devTab >= 0) { side.tab = devTab; if (devQuery !== "") side.setQuery(devQuery) }
    }

    readonly property bool typing: activeFocusItem && activeFocusItem.hasOwnProperty("cursorPosition")
    Shortcut { sequences: ["Right", "Space", "PgDown"]; enabled: !win.typing; onActivated: presenter.nextSlide() }
    Shortcut { sequences: ["Left", "PgUp"]; enabled: !win.typing; onActivated: presenter.prevSlide() }
    Shortcut { sequence: "B"; enabled: !win.typing; onActivated: presenter.blackout = !presenter.blackout }
    Shortcut { sequence: "L"; enabled: !win.typing; onActivated: presenter.logo = !presenter.logo }
    Shortcut { sequence: "T"; enabled: !win.typing; onActivated: presenter.textHidden = !presenter.textHidden }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 12

        // ---- top bar -------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 56; radius: 12; color: T.panel; border.color: T.border
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 10; spacing: 6
                Image {   // BeaconView logo
                    source: "qrc:/assets/logo.png"
                    Layout.preferredHeight: 42
                    Layout.preferredWidth: Math.round(42 * implicitWidth / Math.max(1, implicitHeight))
                    Layout.rightMargin: 24
                    fillMode: Image.PreserveAspectFit
                    smooth: true; mipmap: true
                }
                Repeater {
                    model: ["Dienst", "Liederen", "Media", "Templates", "Schermen", "Rapport", "Instellingen"]
                    AppButton { text: modelData; flat: true; active: win.page === index; implicitHeight: 38
                        onClicked: win.page = index }
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    implicitHeight: 32; implicitWidth: status.implicitWidth + 28; radius: 16; color: "#143a22"
                    Text { id: status; anchors.centerIn: parent; text: "✓ Alles klaar"; color: T.ok; font.pixelSize: 13 }
                }
                AppButton {
                    text: presenter.outputOn ? "● Live" : "Uitvoer starten"
                    primary: !presenter.outputOn; active: presenter.outputOn; implicitHeight: 38
                    onClicked: presenter.outputOn = !presenter.outputOn
                }
            }
        }

        // ---- pages ---------------------------------------------------------------
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: win.page

            RowLayout {
                spacing: 12
                ServicePanel { Layout.preferredWidth: 320; Layout.fillHeight: true }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                    PreviewPanel { Layout.fillWidth: true; Layout.fillHeight: true }
                    SidePanel { id: side; Layout.fillWidth: true; Layout.preferredHeight: 330
                        onEditSongRequested: function (row) { win.page = 1; songsPage.openSong(row) } }
                }
                ControlPanel { Layout.preferredWidth: 340; Layout.fillHeight: true; video: videoEngine
                    onBibleRequested: side.focusBible()
                    onOverlaysRequested: side.tab = 3 }
            }
            SongsPage { id: songsPage }
            MediaPage {}
            TemplatesPage {}
            ScreensPage {}
            ReportPage {}
            SettingsPage {}
        }
    }
}
