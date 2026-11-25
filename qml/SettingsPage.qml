import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "Theme.js" as T

// Settings: the church's own logo (logo screen + watermark).
Item {
    id: root
    property int previewMode: 0   // 0 = logo screen, 1 = slide with watermark
    property string error: ""
    readonly property var b: presenter.branding

    RowLayout {
        anchors.fill: parent; spacing: 12

        // ---- live preview ---------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Voorbeeld"; color: T.text; font.pixelSize: 16; font.bold: true }
                    Item { Layout.fillWidth: true }
                    AppButton { text: "Logo-scherm"; implicitHeight: 32; active: root.previewMode === 0; onClicked: root.previewMode = 0 }
                    AppButton { text: "Slide met watermerk"; implicitHeight: 32; active: root.previewMode === 1; onClicked: root.previewMode = 1 }
                }
                Item {
                    id: holder
                    Layout.fillWidth: true; Layout.fillHeight: true
                    SlideView {
                        anchors.centerIn: parent
                        width: Math.min(holder.width, holder.height * 16 / 9); height: width * 9 / 16
                        showLogo: root.previewMode === 0
                        showWatermark: true
                        background: presenter.defaultBackground
                        text: "De HEERE is mijn Herder,\nmij zal niets ontbreken."
                        reference: "Psalm 23:1"
                    }
                }
            }
        }

        // ---- controls -------------------------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 380; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ScrollView {
                id: sv
                anchors.fill: parent; anchors.margins: 14
                clip: true; contentWidth: availableWidth
                ColumnLayout {
                    width: sv.availableWidth - 12; spacing: 10

                    Text { text: "Logo van de kerk"; color: T.text; font.pixelSize: 18; font.bold: true }
                    Text {
                        Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 12
                        text: root.b.source === ""
                            ? "Nu wordt het standaardkruis gebruikt. Kies een afbeelding met een doorzichtige achtergrond (PNG of SVG) voor het mooiste resultaat."
                            : "Je eigen logo wordt gebruikt voor de Logo-knop en het watermerk."
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; primary: true; text: root.b.source === "" ? "Logo kiezen…" : "Ander logo kiezen…"; onClicked: logoDialog.open() }
                        AppButton { Layout.minimumWidth: 0; text: "Verwijderen"; enabled: root.b.source !== ""; onClicked: { root.error = ""; root.b.clearLogo() } }
                    }
                    Text { visible: root.error !== ""; Layout.fillWidth: true; wrapMode: Text.WordWrap; text: root.error; color: T.danger; font.pixelSize: 12 }

                    Rectangle { Layout.fillWidth: true; height: 1; color: T.border }
                    Text { text: "Logo-scherm"; color: T.text; font.pixelSize: 14; font.bold: true }
                    ColorField { Layout.fillWidth: true; label: "Achtergrondkleur"; value: root.b.bgColor; onPicked: function (c) { root.b.bgColor = c } }
                    RowLayout { Layout.fillWidth: true
                        Text { text: "Grootte"; color: T.muted; font.pixelSize: 12 }
                        Item { Layout.fillWidth: true }
                        Text { text: Math.round(root.b.scale * 100) + "%"; color: T.text; font.pixelSize: 12 } }
                    Slider { Layout.fillWidth: true; from: 0.1; to: 1.0; stepSize: 0.05; value: root.b.scale
                        onMoved: { root.previewMode = 0; root.b.scale = value } }

                    Rectangle { Layout.fillWidth: true; height: 1; color: T.border }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Watermerk op slides"; color: T.text; font.pixelSize: 14; font.bold: true }
                        Item { Layout.fillWidth: true }
                        AppButton { text: root.b.watermark ? "Aan" : "Uit"; active: root.b.watermark; implicitHeight: 32; implicitWidth: 64
                            enabled: root.b.source !== ""
                            onClicked: { root.previewMode = 1; root.b.watermark = !root.b.watermark } }
                    }
                    Text { visible: root.b.source === ""; color: T.muted; font.pixelSize: 12; text: "Kies eerst een logo om een watermerk te kunnen tonen." }
                    GridLayout {
                        Layout.fillWidth: true; columns: 2; rowSpacing: 6; columnSpacing: 6
                        enabled: root.b.source !== "" && root.b.watermark
                        opacity: enabled ? 1 : 0.5
                        Repeater {
                            model: [{ c: 0, t: "Linksboven" }, { c: 1, t: "Rechtsboven" }, { c: 2, t: "Linksonder" }, { c: 3, t: "Rechtsonder" }]
                            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34; text: modelData.t
                                active: root.b.corner === modelData.c; onClicked: { root.previewMode = 1; root.b.corner = modelData.c } }
                        }
                    }
                    RowLayout { Layout.fillWidth: true; opacity: root.b.watermark ? 1 : 0.5
                        Text { text: "Grootte"; color: T.muted; font.pixelSize: 12 }
                        Item { Layout.fillWidth: true }
                        Text { text: Math.round(root.b.watermarkSize * 100) + "%"; color: T.text; font.pixelSize: 12 } }
                    Slider { Layout.fillWidth: true; from: 0.04; to: 0.4; stepSize: 0.01; value: root.b.watermarkSize
                        enabled: root.b.source !== "" && root.b.watermark
                        onMoved: { root.previewMode = 1; root.b.watermarkSize = value } }

                    Rectangle { Layout.fillWidth: true; height: 1; color: T.border }
                    Text { text: "Updates"; color: T.text; font.pixelSize: 14; font.bold: true }
                    Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 12
                        text: "Je gebruikt versie " + updates.currentVersion + ". De app kijkt op GitHub of er een nieuwere versie is en meldt dat bovenin; "
                            + "er wordt niets zelf gedownload of geïnstalleerd, en er wordt niet gekeken zolang de uitvoer open staat." }
                    CheckBox {
                        text: "Automatisch op updates controleren"; checked: updates.autoCheck; onToggled: updates.autoCheck = checked
                        contentItem: Text { text: parent.text; color: T.text; font.pixelSize: 13; leftPadding: parent.indicator.width + 6; verticalAlignment: Text.AlignVCenter }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        AppButton { text: updates.checking ? "Bezig…" : "Nu controleren"; implicitHeight: 34; enabled: !updates.checking; onClicked: updates.check(true) }
                        AppButton { visible: updates.available; text: "Downloaden"; primary: true; implicitHeight: 34; onClicked: updates.openDownload() }
                    }
                    Text { visible: updates.message !== ""; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 12; text: updates.message }

                    Rectangle { Layout.fillWidth: true; height: 1; color: T.border }
                    Text { text: "Problemen oplossen"; color: T.text; font.pixelSize: 14; font.bold: true }
                    Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 12
                        text: "Start een video niet goed? In het logbestand staat wat er gebeurde. Stuur dat mee als je een probleem meldt." }
                    AppButton { Layout.fillWidth: true; implicitHeight: 34; text: "Map met logbestand openen"; onClicked: presenter.openLogFolder() }
                    Text { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; color: T.muted; font.pixelSize: 11; text: presenter.logPath }
                }
            }
        }
    }

    FileDialog {
        id: logoDialog
        title: "Kies het logo van de kerk"
        nameFilters: ["Afbeeldingen (*.png *.svg *.jpg *.jpeg *.webp *.bmp)", "Alle bestanden (*)"]
        onAccepted: {
            if (root.b.chooseLogo(selectedFile)) { root.error = ""; root.previewMode = 0 }
            else root.error = "Dit bestand kan niet als afbeelding worden gelezen."
        }
    }
}
