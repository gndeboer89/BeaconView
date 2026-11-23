import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Window
import "Theme.js" as T

// Generic editor used for service parts and songs.
// In the body text a blank line starts a new slide.
Dialog {
    id: dlg
    property string heading: ""
    property string subtitleLabel: "Ondertitel"
    signal confirmed(string title, string subtitle, string body)

    function openWith(t, s, b) {
        titleField.text = t; subField.text = s; bodyArea.text = b
        open(); titleField.forceActiveFocus()
    }

    modal: true
    // A popup is positioned relative to the item it is declared in. Move it to the window's content item
    // the first time it opens, so it is centred in the whole window (Window.window is not available on a Dialog itself).
    property bool reparented: false
    onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: 560; height: 520
    padding: 20
    background: Rectangle { color: T.panel; radius: 12; border.color: T.border }

    contentItem: ColumnLayout {
        spacing: 10
        Text { text: dlg.heading; color: T.text; font.pixelSize: 20; font.bold: true }
        Text { text: "Titel"; color: T.muted; font.pixelSize: 12 }
        TextField { id: titleField; Layout.fillWidth: true; color: T.text; selectByMouse: true
            background: Rectangle { color: T.card; radius: 6; border.color: titleField.activeFocus ? T.accent : T.border } }
        Text { text: dlg.subtitleLabel; color: T.muted; font.pixelSize: 12 }
        TextField { id: subField; Layout.fillWidth: true; color: T.text; selectByMouse: true
            background: Rectangle { color: T.card; radius: 6; border.color: subField.activeFocus ? T.accent : T.border } }
        Text { text: "Tekst (een lege regel = nieuwe slide)"; color: T.muted; font.pixelSize: 12 }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            TextArea { id: bodyArea; color: T.text; selectByMouse: true; wrapMode: TextEdit.Wrap
                background: Rectangle { color: T.card; radius: 6; border.color: bodyArea.activeFocus ? T.accent : T.border } }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight; spacing: 8
            AppButton { text: "Annuleren"; onClicked: dlg.close() }
            AppButton { text: "Opslaan"; primary: true; enabled: titleField.text.trim().length > 0
                onClicked: { dlg.confirmed(titleField.text.trim(), subField.text.trim(), bodyArea.text); dlg.close() } }
        }
    }
}
