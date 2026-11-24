import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import "Theme.js" as T

// Options of a video part: title, file, repeat and "continue when finished".
Dialog {
    id: dlg
    property int itemIndex: -1
    property url file: ""
    signal confirmed(int index, string title, url file, bool loop, bool next)

    function openFor(i) {
        var info = presenter.itemInfo(i)
        itemIndex = i
        titleField.text = info.title
        file = info.video
        loopBox.checked = info.videoLoop
        nextBox.checked = info.videoNext
        open(); titleField.forceActiveFocus()
    }

    modal: true
    property bool reparented: false
    onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: 520; padding: 20
    background: Rectangle { color: T.panel; radius: 12; border.color: T.border }

    contentItem: ColumnLayout {
        spacing: 10
        Text { text: "Video"; color: T.text; font.pixelSize: 20; font.bold: true }
        Text { text: "Titel (in de dienstlijst)"; color: T.muted; font.pixelSize: 12 }
        TextField { id: titleField; Layout.fillWidth: true; color: T.text; selectByMouse: true
            background: Rectangle { color: T.card; radius: 6; border.color: titleField.activeFocus ? T.accent : T.border } }
        Text { text: "Bestand"; color: T.muted; font.pixelSize: 12 }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Text { Layout.fillWidth: true; elide: Text.ElideMiddle; font.pixelSize: 13
                color: presenter.fileExists(dlg.file.toString()) ? T.text : T.danger
                text: (presenter.fileExists(dlg.file.toString()) ? "" : "⚠ Niet gevonden: ") + decodeURIComponent(dlg.file.toString().replace(/^file:\/+/, "/")) }
            AppButton { text: "Ander bestand…"; implicitHeight: 34; onClicked: fileDialog.open() }
        }
        CheckBox { id: loopBox; text: "Herhalen zolang dit onderdeel live staat"
            contentItem: Text { text: loopBox.text; color: T.text; font.pixelSize: 13; leftPadding: loopBox.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
        CheckBox { id: nextBox; text: "Na afloop automatisch naar het volgende onderdeel"
            enabled: !loopBox.checked
            contentItem: Text { text: nextBox.text; color: nextBox.enabled ? T.text : T.muted; font.pixelSize: 13; leftPadding: nextBox.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
        RowLayout {
            Layout.alignment: Qt.AlignRight; spacing: 8
            AppButton { text: "Annuleren"; onClicked: dlg.close() }
            AppButton { text: "Opslaan"; primary: true; onClicked: { dlg.confirmed(dlg.itemIndex, titleField.text.trim(), dlg.file, loopBox.checked, nextBox.checked && !loopBox.checked); dlg.close() } }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Kies een video"
        nameFilters: ["Video's (*.mp4 *.m4v *.mov *.webm *.mkv *.avi *.wmv)", "Alle bestanden (*)"]
        onAccepted: dlg.file = selectedFile
    }
}
