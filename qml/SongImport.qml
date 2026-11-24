import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import "Theme.js" as T

// Import songs from SongSelect downloads (.txt, .usr): choose the files, then a message says what was added.
Item {
    id: root
    signal imported(var result)
    function open() { fileDialog.open() }

    FileDialog {
        id: fileDialog
        title: "Kies liedteksten uit SongSelect (.txt of .usr)"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Liedteksten (*.txt *.usr)", "Alle bestanden (*)"]
        onAccepted: {
            var r = presenter.songs.importFiles(selectedFiles)
            var parts = []
            if (r.added > 0) parts.push(r.added + (r.added === 1 ? " lied toegevoegd" : " liederen toegevoegd"))
            if (r.existing > 0) parts.push(r.existing + (r.existing === 1 ? " stond er al in" : " stonden er al in"))
            if (r.failed > 0) parts.push(r.failed + (r.failed === 1 ? " bestand kon niet worden gelezen" : " bestanden konden niet worden gelezen"))
            result.text = parts.join(";  ")
            result.detail = r.failed > 0 ? "Gebruik het tekstbestand (.txt) of het USR-bestand (.usr) van SongSelect." : (r.titles.length > 0 ? r.titles.join("\n") : "")
            result.open()
            root.imported(r)
        }
    }
    Dialog {
        id: result
        property string text: ""
        property string detail: ""
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Liederen importeren"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.text; font.pixelSize: 14; text: result.text }
            Text { visible: result.detail !== ""; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 13; text: result.detail }
            AppButton { Layout.alignment: Qt.AlignRight; text: "OK"; primary: true; onClicked: result.close() }
        }
    }
}
