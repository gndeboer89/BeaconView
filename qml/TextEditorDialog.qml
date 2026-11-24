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
    property string bodyLabel: "Tekst (een lege regel = nieuwe slide)"
    signal confirmed(string title, string subtitle, string body)
    // Songs: the order in which the parts ("[Couplet 1]", "[Refrein]") are sung. Empty = as written.
    property bool showOrder: false
    property string orderLabel: "Volgorde van de delen"
    property string resetLabel: ""          // text of the button that puts the order back (hidden when empty)
    property string resetValue: ""          // ...to this order
    readonly property string orderValue: orderField.text.trim()
    // Songs: the bundle (one) and the tags (as many as you like), as chosen in the library.
    property bool showMeta: false
    readonly property string categoryValue: categoryField.text.trim()
    readonly property var tagsValue: tagsField.text.split(/[,;\n]/).map(function (t) { return t.trim() }).filter(function (t) { return t !== "" })

    function openWith(t, s, b, order, category, tags) {
        titleField.text = t; subField.text = s; bodyArea.text = b
        orderField.text = order !== undefined ? order : ""
        categoryField.text = category !== undefined ? category : ""
        tagsField.text = tags !== undefined ? tags.join(", ") : ""
        open(); titleField.forceActiveFocus()
    }

    modal: true
    // A popup is positioned relative to the item it is declared in. Move it to the window's content item
    // the first time it opens, so it is centred in the whole window (Window.window is not available on a Dialog itself).
    property bool reparented: false
    onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    width: 560
    height: Math.min(showOrder ? (showMeta ? 780 : 640) : 520, parent ? parent.height - 30 : 520)   // never taller than the window
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
        Text { text: dlg.bodyLabel; color: T.muted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true
            TextArea { id: bodyArea; color: T.text; selectByMouse: true; wrapMode: TextEdit.Wrap
                background: Rectangle { color: T.card; radius: 6; border.color: bodyArea.activeFocus ? T.accent : T.border } }
        }
        // bundle and tags
        ColumnLayout {
            visible: dlg.showMeta; Layout.fillWidth: true; spacing: 6
            Text { text: "Bundel (bijvoorbeeld Opwekking, Hillsong of Liedboek)"; color: T.muted; font.pixelSize: 12 }
            TextField { id: categoryField; Layout.fillWidth: true; color: T.text; selectByMouse: true; font.pixelSize: 13
                background: Rectangle { color: T.card; radius: 6; border.color: categoryField.activeFocus ? T.accent : T.border } }
            Flow {
                Layout.fillWidth: true; spacing: 6
                Repeater {
                    model: dlg.showMeta ? presenter.songs.categories.slice(0, 12) : []
                    AppButton { required property string modelData; text: modelData; implicitHeight: 28; leftPadding: 10; rightPadding: 10
                        active: categoryField.text.trim().toLowerCase() === modelData.toLowerCase()
                        onClicked: categoryField.text = modelData }
                }
            }
            Text { text: "Tags, gescheiden door komma's (bijvoorbeeld Kerst, Pasen)"; color: T.muted; font.pixelSize: 12 }
            TextField { id: tagsField; Layout.fillWidth: true; color: T.text; selectByMouse: true; font.pixelSize: 13
                background: Rectangle { color: T.card; radius: 6; border.color: tagsField.activeFocus ? T.accent : T.border } }
            Flow {
                Layout.fillWidth: true; spacing: 6
                Repeater {
                    model: dlg.showMeta ? presenter.songs.tags.slice(0, 16) : []
                    AppButton {
                        required property string modelData
                        readonly property bool on: dlg.tagsValue.some(function (t) { return t.toLowerCase() === modelData.toLowerCase() })
                        text: (on ? "✓ " : "+ ") + modelData; implicitHeight: 28; leftPadding: 10; rightPadding: 10; active: on
                        onClicked: {
                            var cur = dlg.tagsValue, key = modelData.toLowerCase()
                            tagsField.text = (on ? cur.filter(function (t) { return t.toLowerCase() !== key }) : cur.concat([modelData])).join(", ")
                        }
                    }
                }
            }
        }
        // order of the parts
        ColumnLayout {
            visible: dlg.showOrder; Layout.fillWidth: true; spacing: 6
            Text { text: dlg.orderLabel; color: T.muted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                TextField { id: orderField; Layout.fillWidth: true; color: T.text; selectByMouse: true; font.pixelSize: 13
                    placeholderText: "leeg = de delen zoals ze hierboven staan"
                    background: Rectangle { color: T.card; radius: 6; border.color: orderField.activeFocus ? T.accent : T.border } }
                AppButton { visible: dlg.resetLabel !== ""; text: dlg.resetLabel; implicitHeight: 34
                    onClicked: orderField.text = dlg.resetValue }
            }
            Flow {
                Layout.fillWidth: true; spacing: 6
                Text { text: "Voeg toe:"; color: T.muted; font.pixelSize: 12; height: 30; verticalAlignment: Text.AlignVCenter
                    visible: partsRepeater.count > 0 }
                Repeater {
                    id: partsRepeater
                    model: dlg.showOrder ? presenter.partNames(bodyArea.text) : []
                    AppButton { required property string modelData; text: "+ " + modelData; implicitHeight: 30; leftPadding: 10; rightPadding: 10
                        onClicked: orderField.text = (orderField.text.trim() === "" ? "" : orderField.text.trim() + ", ") + modelData }
                }
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight; spacing: 8
            AppButton { text: "Annuleren"; onClicked: dlg.close() }
            AppButton { text: "Opslaan"; primary: true; enabled: titleField.text.trim().length > 0
                onClicked: { dlg.confirmed(titleField.text.trim(), subField.text.trim(), bodyArea.text); dlg.close() } }
        }
    }
}
