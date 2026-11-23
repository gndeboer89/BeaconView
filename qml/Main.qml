import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

ApplicationWindow {
    id: win
    width: 1360; height: 820; minimumWidth: 1200; minimumHeight: 700
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

    OutputWindow {}
    Component.onCompleted: {   // developer aid: --tab N --query "Joh 3:16"
        if (devTab >= 0) { side.tab = devTab; if (devQuery !== "") side.setQuery(devQuery) }
    }

    readonly property bool typing: activeFocusItem && activeFocusItem.hasOwnProperty("cursorPosition")
    Shortcut { sequences: ["Right", "Space", "PgDown"]; enabled: !win.typing; onActivated: presenter.nextSlide() }
    Shortcut { sequences: ["Left", "PgUp"]; enabled: !win.typing; onActivated: presenter.prevSlide() }
    Shortcut { sequence: "B"; enabled: !win.typing; onActivated: presenter.blackout = !presenter.blackout }
    Shortcut { sequence: "L"; enabled: !win.typing; onActivated: presenter.logo = !presenter.logo }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 12

        // ---- top bar -------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 56; radius: 12; color: T.panel; border.color: T.border
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 10; spacing: 6
                Text { text: "✝"; color: T.accent; font.pixelSize: 24 }
                Text { text: "BeaconView"; color: T.text; font.pixelSize: 18; font.bold: true; Layout.rightMargin: 24 }
                Repeater {
                    model: ["Dienst", "Media", "Templates", "Schermen", "Instellingen"]
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
                ServicePanel { Layout.preferredWidth: 340; Layout.fillHeight: true }
                PreviewPanel { Layout.fillWidth: true; Layout.fillHeight: true
                    onBibleRequested: { side.tab = 2; side.focusBible() } }
                SidePanel { id: side; Layout.preferredWidth: 380; Layout.minimumWidth: 380; Layout.fillHeight: true }
            }
            Placeholder { title: "Media" ; note: "Mediabibliotheek (video's, afbeeldingen) – komt in een volgende versie. Achtergronden vind je nu rechts in Dienst." }
            Placeholder { title: "Templates"; note: "Lettertype, kleuren en layout per template – komt in een volgende versie." }
            ScreensPage {}
            Placeholder { title: "Instellingen" }
        }
    }
}
