import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// A colour as quick swatches plus a hex field (#rrggbb).
Item {
    id: cf
    property string label: ""
    property string value: "#ffffff"
    signal picked(string color)
    implicitHeight: col.implicitHeight
    implicitWidth: col.implicitWidth

    ColumnLayout {
        id: col
        anchors.left: parent.left; anchors.right: parent.right
        spacing: 4
        Text { text: cf.label; color: T.muted; font.pixelSize: 12 }
        RowLayout {
            Layout.fillWidth: true; spacing: 5
            Repeater {
                model: ["#ffffff", "#fdf6e3", "#ffe45c", "#9ed8ff", "#ffb3b3", "#c8f7c5", "#000000"]
                Rectangle {
                    width: 22; height: 22; radius: 11; color: modelData
                    border.width: cf.value.toLowerCase() === modelData ? 2 : 1
                    border.color: cf.value.toLowerCase() === modelData ? T.accent : T.border
                    opacity: cf.enabled ? 1 : 0.5
                    TapHandler { enabled: cf.enabled; onTapped: cf.picked(modelData) }
                }
            }
            Item { Layout.fillWidth: true }
            TextField {
                id: hex
                Layout.preferredWidth: 84
                text: cf.value; color: T.text; font.pixelSize: 13; selectByMouse: true
                maximumLength: 7
                validator: RegularExpressionValidator { regularExpression: /^#[0-9a-fA-F]{6}$/ }
                background: Rectangle { color: T.card; radius: 6; border.color: hex.activeFocus ? (hex.acceptableInput ? T.accent : T.danger) : T.border }
                onEditingFinished: if (acceptableInput) cf.picked(text.toLowerCase())
            }
        }
    }
}
