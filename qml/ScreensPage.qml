import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

Item {
    id: root
    ScrollView {
        id: scroll
        anchors.fill: parent; contentWidth: availableWidth; clip: true
    ColumnLayout {
        x: 24; width: scroll.availableWidth - 48; spacing: 14
        Item { Layout.preferredHeight: 10 }
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

        // ---- extra screens -----------------------------------------------------------------
        Rectangle { Layout.fillWidth: true; Layout.maximumWidth: 720; Layout.topMargin: 10; height: 1; color: T.border }
        Text { text: "Extra schermen"; color: T.text; font.pixelSize: 20; font.bold: true }
        Text {
            Layout.fillWidth: true; Layout.maximumWidth: 720; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
            text: "Een stage-scherm toont voor wie op het podium staat de huidige en de volgende tekst, de klok en de timer. "
                + "Een ondertitelscherm toont alleen de tekst op een effen kleur om uit te keyen (chroma-key) in een videomixer of streamprogramma. "
                + "Zonder gekozen beeldscherm opent het een los venster: sleep het naar een scherm, of laat het opnemen met een venster-opname (de venstertitel is de naam)."
        }
        RowLayout {
            spacing: 8
            AppButton { text: "+  Stage-scherm"; onClicked: presenter.extraScreens.add("stage") }
            AppButton { text: "+  Ondertitels (chroma-key)"; onClicked: presenter.extraScreens.add("subtitles") }
        }

        Repeater {
            model: presenter.extraScreens
            delegate: Rectangle {
                id: card
                required property int index
                required property string name
                required property string kind
                required property string screenName
                required property bool on
                required property bool autoOpen
                required property string chromaColor
                required property string textColor
                required property real scale
                required property string position
                required property bool showNext
                required property bool showClock
                required property bool showTimer
                readonly property bool stage: kind === "stage"
                Layout.fillWidth: true; Layout.maximumWidth: 720
                implicitHeight: content.implicitHeight + 28
                radius: 10; color: T.panel; border.color: on ? T.accent : T.border; border.width: on ? 2 : 1

                RowLayout {
                    id: content
                    anchors.fill: parent; anchors.margins: 14; spacing: 16

                    Rectangle {   // live preview
                        Layout.preferredWidth: 256; Layout.preferredHeight: 144; Layout.alignment: Qt.AlignTop
                        color: "black"; border.color: T.border
                        Loader {
                            anchors.fill: parent; anchors.margins: 1
                            sourceComponent: card.stage ? stagePreview : subtitlePreview
                        }
                        Component { id: stagePreview; StageView { scale1: card.scale; showNext: card.showNext; showClock: card.showClock; showTimer: card.showTimer } }
                        Component { id: subtitlePreview; SubtitleView { chroma: card.chromaColor; textColor: card.textColor; scale1: card.scale; position: card.position
                            sampleText: "Zo ziet de ondertitel eruit" } }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 8
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            TextField {
                                Layout.fillWidth: true; text: card.name; color: T.text; selectByMouse: true; font.pixelSize: 15
                                background: Rectangle { color: T.card; radius: 6; border.color: parent.activeFocus ? T.accent : T.border }
                                onEditingFinished: if (text.trim() !== "") presenter.extraScreens.set(card.index, "name", text.trim())
                            }
                            AppButton { text: card.on ? "✓ Open" : "Openen"; active: card.on
                                onClicked: presenter.extraScreens.set(card.index, "on", !card.on) }
                            AppButton { text: "✕"; flat: true; implicitWidth: 34; onClicked: presenter.extraScreens.remove(card.index) }
                        }
                        Text { text: card.stage ? "Stage-scherm" : "Ondertitels (chroma-key)"; color: T.muted; font.pixelSize: 12 }

                        ComboBox {
                            id: monitor
                            Layout.fillWidth: true; font.pixelSize: 13
                            model: {
                                var m = ["Los venster"]
                                for (var i = 0; i < Qt.application.screens.length; ++i)
                                    m.push(Qt.application.screens[i].name + "  (" + Qt.application.screens[i].width + " × " + Qt.application.screens[i].height + ")")
                                return m
                            }
                            currentIndex: {
                                for (var i = 0; i < Qt.application.screens.length; ++i)
                                    if (card.screenName !== "" && Qt.application.screens[i].name === card.screenName) return i + 1
                                return 0
                            }
                            onActivated: presenter.extraScreens.set(card.index, "screenName", currentIndex === 0 ? "" : Qt.application.screens[currentIndex - 1].name)
                        }

                        // stage options
                        Flow {
                            visible: card.stage; Layout.fillWidth: true; spacing: 14
                            CheckBox { text: "Volgende tekst"; checked: card.showNext; onToggled: presenter.extraScreens.set(card.index, "showNext", checked)
                                contentItem: Text { text: parent.text; color: T.text; font.pixelSize: 13; leftPadding: parent.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
                            CheckBox { text: "Klok"; checked: card.showClock; onToggled: presenter.extraScreens.set(card.index, "showClock", checked)
                                contentItem: Text { text: parent.text; color: T.text; font.pixelSize: 13; leftPadding: parent.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
                            CheckBox { text: "Timer"; checked: card.showTimer; onToggled: presenter.extraScreens.set(card.index, "showTimer", checked)
                                contentItem: Text { text: parent.text; color: T.text; font.pixelSize: 13; leftPadding: parent.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
                        }

                        // subtitle options
                        ColumnLayout {
                            visible: !card.stage; Layout.fillWidth: true; spacing: 6
                            Text { text: "Achtergrondkleur (chroma-key)"; color: T.muted; font.pixelSize: 12 }
                            RowLayout {
                                spacing: 6
                                Repeater {
                                    model: [{ n: "Groen", c: "#00b140" }, { n: "Blauw", c: "#0047bb" }, { n: "Magenta", c: "#ff00ff" }, { n: "Zwart", c: "#000000" }]
                                    Rectangle {
                                        required property var modelData
                                        width: 74; height: 28; radius: 6; color: modelData.c
                                        border.color: card.chromaColor === modelData.c ? T.accent : T.border; border.width: card.chromaColor === modelData.c ? 3 : 1
                                        Text { anchors.centerIn: parent; text: modelData.n; font.pixelSize: 12; font.bold: true
                                            color: modelData.c === "#ffff00" ? "black" : "white"; style: Text.Outline; styleColor: "#66000000" }
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                            onClicked: presenter.extraScreens.set(card.index, "chromaColor", modelData.c) }
                                    }
                                }
                            }
                            ColorField { Layout.fillWidth: true; label: "Tekstkleur"; value: card.textColor
                                onPicked: function (c) { presenter.extraScreens.set(card.index, "textColor", c) } }
                            RowLayout {
                                spacing: 8
                                Text { text: "Positie"; color: T.muted; font.pixelSize: 12 }
                                ComboBox {
                                    model: ["Onder", "Midden", "Boven"]; font.pixelSize: 13; implicitWidth: 120
                                    currentIndex: card.position === "top" ? 2 : card.position === "middle" ? 1 : 0
                                    onActivated: presenter.extraScreens.set(card.index, "position", ["bottom", "middle", "top"][currentIndex])
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            Text { text: "Tekstgrootte"; color: T.muted; font.pixelSize: 12 }
                            Slider { Layout.fillWidth: true; from: 0.5; to: 2.0; stepSize: 0.05; value: card.scale
                                onMoved: presenter.extraScreens.set(card.index, "scale", value) }
                            Text { text: Math.round(card.scale * 100) + "%"; color: T.muted; font.pixelSize: 12; Layout.preferredWidth: 40 }
                        }
                        CheckBox {
                            text: "Open mee met de uitvoer"; checked: card.autoOpen; onToggled: presenter.extraScreens.set(card.index, "autoOpen", checked)
                            contentItem: Text { text: parent.text; color: T.text; font.pixelSize: 13; leftPadding: parent.indicator.width + 6; verticalAlignment: Text.AlignVCenter }
                        }
                    }
                }
            }
        }
        Item { Layout.preferredHeight: 24 }
    }
    }
}
