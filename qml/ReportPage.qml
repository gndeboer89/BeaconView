import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import "Theme.js" as T

// Report of the songs that were shown to the congregation (for example for the CCLI report).
Item {
    id: root
    property string period: "month"
    property var rows: presenter.usage.summary(period)
    property string message: ""
    Connections { target: presenter.usage; function onChanged() { root.rows = presenter.usage.summary(root.period) } }
    onPeriodChanged: { rows = presenter.usage.summary(period); message = "" }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 12

        Text { text: "Liederenrapport"; color: T.text; font.pixelSize: 24; font.bold: true }
        Text {
            Layout.fillWidth: true; Layout.maximumWidth: 900; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
            text: "Welke liederen zijn getoond aan de gemeente. Een lied telt één keer per keer dat de uitvoer is geopend, "
                + "zodra het op het scherm komt; oefenen zonder uitvoer telt niet. Handig voor de opgave aan CCLI."
        }

        RowLayout {
            spacing: 8
            Repeater {
                model: [{ k: "month", n: "Deze maand" }, { k: "lastmonth", n: "Vorige maand" }, { k: "year", n: "Dit jaar" }, { k: "all", n: "Alles" }]
                AppButton { required property var modelData; text: modelData.n; active: root.period === modelData.k; implicitHeight: 34
                    onClicked: root.period = modelData.k }
            }
            Item { Layout.fillWidth: true }
            AppButton { text: "Exporteren als CSV…"; primary: true; implicitHeight: 34; enabled: root.rows.length > 0; onClicked: exportDialog.open() }
            AppButton { text: "Gegevens wissen…"; implicitHeight: 34; onClicked: clearDialog.open() }
        }

        Text {
            color: T.text; font.pixelSize: 14
            text: root.rows.length === 0 ? "Geen liederen in " + presenter.usage.periodText(root.period)
                : root.rows.length + (root.rows.length === 1 ? " lied" : " liederen") + ", samen " + presenter.usage.uses(root.period) + " keer gebruikt in "
                  + presenter.usage.periodText(root.period)
        }
        Text { visible: root.message !== ""; color: T.ok; font.pixelSize: 13; text: root.message }

        // table
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.maximumWidth: 900
            color: T.panel; radius: 10; border.color: T.border; clip: true
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 1; spacing: 0
                Rectangle {
                    Layout.fillWidth: true; height: 34; color: T.card
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 10
                        Text { text: "Lied"; color: T.muted; font.pixelSize: 12; font.bold: true; Layout.fillWidth: true; Layout.preferredWidth: 4 }
                        Text { text: "CCLI-nummer"; color: T.muted; font.pixelSize: 12; font.bold: true; Layout.preferredWidth: 110 }
                        Text { text: "Auteur"; color: T.muted; font.pixelSize: 12; font.bold: true; Layout.fillWidth: true; Layout.preferredWidth: 3 }
                        Text { text: "Keer"; color: T.muted; font.pixelSize: 12; font.bold: true; Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight }
                        Text { text: "Laatst"; color: T.muted; font.pixelSize: 12; font.bold: true; Layout.preferredWidth: 90 }
                    }
                }
                ListView {
                    id: list
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    model: root.rows
                    ScrollBar.vertical: ScrollBar {}
                    Text {
                        visible: root.rows.length === 0
                        anchors.centerIn: parent; color: T.muted; font.pixelSize: 14
                        text: "Nog niets in dit rapport."
                    }
                    delegate: Rectangle {
                        required property int index
                        required property var modelData
                        width: list.width; height: 38; color: index % 2 ? "transparent" : "#0b0f16"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 10
                            Text { text: modelData.title; color: T.text; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true; Layout.preferredWidth: 4 }
                            Text { text: modelData.ccli !== "" ? modelData.ccli : "–"; color: T.muted; font.pixelSize: 13; Layout.preferredWidth: 110 }
                            Text { text: modelData.author; color: T.muted; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true; Layout.preferredWidth: 3 }
                            Text { text: modelData.count; color: T.text; font.pixelSize: 14; font.bold: true; Layout.preferredWidth: 44; horizontalAlignment: Text.AlignRight }
                            Text { text: modelData.last; color: T.muted; font.pixelSize: 13; Layout.preferredWidth: 90 }
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: exportDialog
        title: "Rapport opslaan"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "csv"
        nameFilters: ["CSV-bestanden (*.csv)", "Alle bestanden (*)"]
        onAccepted: root.message = presenter.usage.exportCsv(selectedFile, root.period)
                    ? "Opgeslagen. Open het bestand in Excel of een ander rekenprogramma."
                    : "Het bestand kon niet worden opgeslagen."
    }
    Dialog {
        id: clearDialog
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Alle gegevens wissen?"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: "Het hele rapport (ook de andere perioden) wordt gewist. Exporteer het eerst als je het nodig hebt. Je liederen en diensten blijven staan." }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: clearDialog.close() }
                AppButton { text: "Wissen"; primary: true; onClicked: { presenter.usage.clear(); root.message = ""; clearDialog.close() } }
            }
        }
    }
}
