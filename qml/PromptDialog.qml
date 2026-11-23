import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Window
import "Theme.js" as T

// Small dialog asking for one line of text.
Dialog {
    id: dlg
    property string heading: ""
    property string fieldLabel: ""
    signal confirmed(string text)

    function openWith(text) { field.text = text; open(); field.forceActiveFocus(); field.selectAll() }

    modal: true
    // A popup is positioned relative to the item it is declared in. Move it to the window's content item
    // the first time it opens, so it is centred in the whole window (Window.window is not available on a Dialog itself).
    property bool reparented: false
    onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: 520; padding: 20
    background: Rectangle { color: T.panel; radius: 12; border.color: T.border }

    contentItem: ColumnLayout {
        spacing: 10
        Text { text: dlg.heading; color: T.text; font.pixelSize: 20; font.bold: true }
        Text { text: dlg.fieldLabel; color: T.muted; font.pixelSize: 12 }
        TextField {
            id: field; Layout.fillWidth: true; color: T.text; selectByMouse: true
            background: Rectangle { color: T.card; radius: 6; border.color: field.activeFocus ? T.accent : T.border }
            onAccepted: if (text.trim() !== "") { dlg.confirmed(text.trim()); dlg.close() }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight; spacing: 8
            AppButton { text: "Annuleren"; onClicked: dlg.close() }
            AppButton { text: "Opslaan"; primary: true; enabled: field.text.trim() !== ""
                onClicked: { dlg.confirmed(field.text.trim()); dlg.close() } }
        }
    }
}
