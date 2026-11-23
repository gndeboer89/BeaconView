import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

Item {
    id: root
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 14
        Text { text: "Schermen"; color: T.text; font.pixelSize: 24; font.bold: true }
        Text {
            Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
            text: Qt.application.screens.length < 2
                ? "Er is maar één scherm gevonden. De uitvoer opent nu in een los venster; sluit een beamer, tv of LED-wand aan om volledig scherm te projecteren."
                : "Kies op welk scherm de presentatie wordt geprojecteerd."
        }
        Repeater {
            model: Qt.application.screens
            Rectangle {
                Layout.fillWidth: true; Layout.maximumWidth: 720; height: 72; radius: 10
                readonly property bool chosen: presenter.outputScreen === modelData.name
                    || (presenter.outputScreen === "" && index === (Qt.application.screens.length > 1 ? 1 : 0))
                color: T.panel; border.color: chosen ? T.accent : T.border; border.width: chosen ? 2 : 1
                RowLayout {
                    anchors.fill: parent; anchors.margins: 14; spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 2
                        Text { text: modelData.name + (index === 0 ? "  (hoofdscherm)" : ""); color: T.text; font.pixelSize: 16 }
                        Text { text: modelData.width + " × " + modelData.height; color: T.muted; font.pixelSize: 13 }
                    }
                    AppButton { text: chosen ? "✓ Uitvoerscherm" : "Gebruik als uitvoer"; active: chosen
                        onClicked: presenter.outputScreen = modelData.name }
                }
            }
        }
        AppButton {
            text: presenter.outputOn ? "Uitvoer sluiten" : "Uitvoer openen"; primary: !presenter.outputOn
            onClicked: presenter.outputOn = !presenter.outputOn
        }
        Item { Layout.fillHeight: true }
    }
}
