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
    // The cross of the window asks first: closing by accident in the middle of a service is expensive.
    property bool quitConfirmed: false
    onClosing: function (close) {
        if (quitConfirmed) { Qt.quit(); return }
        close.accepted = false
        quitDialog.open()
    }

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

    Dialog {
        id: quitDialog
        modal: true
        x: Math.round((win.width - width) / 2); y: Math.round((win.height - height) / 2)
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        onOpened: quitButton.forceActiveFocus()          // Enter confirms, Escape cancels
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "BeaconView afsluiten?"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: (presenter.outputOn ? "De uitvoer staat nog open en wordt gesloten. " : "")
                    + (songsPage.changed ? "Er zijn wijzigingen in een lied die nog niet zijn opgeslagen; die gaan verloren. " : "")
                    + "Weet je zeker dat je het programma wilt afsluiten?"
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: quitDialog.close() }
                AppButton { id: quitButton; text: "Afsluiten"; primary: true
                    Keys.onReturnPressed: clicked(); Keys.onEnterPressed: clicked()
                    onClicked: { win.quitConfirmed = true; win.close() } }
            }
        }
    }

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

        // ---- a newer version exists (only a message: nothing is downloaded by the program) ----
        Rectangle {
            visible: updates.bannerVisible
            Layout.fillWidth: true; Layout.preferredHeight: 44; radius: 10; color: T.accentSoft; border.color: T.accent
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 8; spacing: 10
                Text { Layout.fillWidth: true; color: T.text; font.pixelSize: 14; elide: Text.ElideRight
                    text: "Versie " + updates.latestVersion + " is beschikbaar (je gebruikt " + updates.currentVersion + ")." }
                AppButton { text: "Downloaden"; primary: true; implicitHeight: 30; onClicked: updates.openDownload() }
                AppButton { text: "Deze versie overslaan"; implicitHeight: 30; onClicked: updates.skipVersion() }
                AppButton { text: "✕"; flat: true; implicitWidth: 30; implicitHeight: 30; onClicked: updates.closeBanner() }
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
