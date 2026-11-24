import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// Progress of a background import (PDF / PowerPoint / pictures) and its error message, if any.
Dialog {
    id: dlg
    property string error: ""
    readonly property var imp: presenter.importer

    modal: true
    closePolicy: imp.busy ? Popup.NoAutoClose : Popup.CloseOnEscape
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: 460; padding: 20
    background: Rectangle { color: T.panel; radius: 12; border.color: T.border }

    Connections {
        target: dlg.imp
        function onBusyChanged() { if (dlg.imp.busy) { dlg.error = ""; dlg.open() } }
        function onFinished() { dlg.close() }
        function onFailed(tag, message) { dlg.error = message; dlg.open() }
    }

    contentItem: ColumnLayout {
        spacing: 12
        Text { text: dlg.error !== "" ? "Importeren is niet gelukt" : "Slides importeren"; color: dlg.error !== "" ? T.danger : T.text; font.pixelSize: 20; font.bold: true }
        Text {
            Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 14
            color: dlg.error !== "" ? T.text : T.muted
            text: dlg.error !== "" ? dlg.error : dlg.imp.status
        }
        ProgressBar {
            Layout.fillWidth: true; visible: dlg.error === ""
            indeterminate: dlg.imp.total <= 0
            from: 0; to: Math.max(1, dlg.imp.total); value: dlg.imp.done
        }
        AppButton { visible: dlg.error !== ""; Layout.alignment: Qt.AlignRight; text: "Sluiten"; primary: true; onClicked: dlg.close() }
    }
}
