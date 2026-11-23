import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// Template manager: list on the left, live preview in the middle, editor on the right.
Item {
    id: root
    property string selId: presenter.templates.activeId
    // Re-evaluated whenever the library changes
    property var draft: { presenter.templates.all; return presenter.templates.get(selId) }
    readonly property bool editable: !draft.builtin
    property bool longSample: false
    property int leftMode: devLeft   // 0 = templates, 1 = default per kind of part

    function setKey(k, v) {
        var o = {}
        o[k] = v
        presenter.templates.update(selId, o)
    }

    RowLayout {
        anchors.fill: parent; spacing: 12

        // ---- list / defaults per kind -----------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 290; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 10

                RowLayout {
                    Layout.fillWidth: true; spacing: 6
                    AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34; text: "Templates"; active: root.leftMode === 0; onClicked: root.leftMode = 0 }
                    AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34; text: "Per onderdeel"; active: root.leftMode === 1; onClicked: root.leftMode = 1 }
                }

                StackLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    currentIndex: root.leftMode

                    // -- templates --
                    ColumnLayout {
                        spacing: 10
                        ListView {
                            id: list
                            Layout.fillWidth: true; Layout.fillHeight: true
                            clip: true; spacing: 4
                            model: presenter.templates.all
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Rectangle {
                                width: list.width - 8; height: 52; radius: 8
                                readonly property bool selected: modelData.id === root.selId
                                color: selected ? T.accentSoft : (hov.hovered ? T.card : "transparent")
                                border.color: selected ? T.accent : "transparent"
                                HoverHandler { id: hov }
                                TapHandler { onTapped: root.selId = modelData.id }
                                RowLayout {
                                    anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 10; spacing: 8
                                    ColumnLayout {
                                        Layout.fillWidth: true; spacing: 0
                                        Text { text: modelData.name; color: T.text; font.pixelSize: 14; elide: Text.ElideRight; Layout.fillWidth: true }
                                        Text { text: modelData.builtin ? "Ingebouwd" : "Eigen template"; color: T.muted; font.pixelSize: 11 }
                                    }
                                    Text { visible: modelData.id === presenter.templates.activeId; text: "● actief"; color: T.ok; font.pixelSize: 12 }
                                }
                            }
                        }
                        AppButton {
                            Layout.fillWidth: true; text: root.selId === presenter.templates.activeId ? "✓ Actief template" : "Gebruik dit template"
                            primary: root.selId !== presenter.templates.activeId
                            enabled: root.selId !== presenter.templates.activeId
                            onClicked: presenter.templates.setActive(root.selId)
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 8
                            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Dupliceren"
                                onClicked: root.selId = presenter.templates.duplicate(root.selId) }
                            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Verwijderen"; enabled: root.editable
                                onClicked: { var id = root.selId; root.selId = presenter.templates.activeId === id ? "builtin:classic" : presenter.templates.activeId; presenter.templates.remove(id) } }
                        }
                    }

                    // -- default template per kind of part --
                    ColumnLayout {
                        spacing: 8
                        Text {
                            Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 12
                            text: "Kies welk template elke soort onderdeel standaard gebruikt. Een eigen keuze bij een onderdeel (via het menu bij dat onderdeel) gaat voor."
                        }
                        Repeater {
                            model: [{ k: "welcome", t: "Welkom" }, { k: "song", t: "Lied" }, { k: "prayer", t: "Gebed" },
                                    { k: "scripture", t: "Schriftlezing" }, { k: "sermon", t: "Verkondiging" },
                                    { k: "offering", t: "Collecte" }, { k: "blessing", t: "Zegen" }, { k: "custom", t: "Eigen tekst" }]
                            RowLayout {
                                Layout.fillWidth: true; spacing: 8
                                Text { text: modelData.t; color: T.text; font.pixelSize: 13; Layout.preferredWidth: 92 }
                                ComboBox {
                                    id: kindBox
                                    Layout.fillWidth: true; Layout.minimumWidth: 0; font.pixelSize: 12
                                    textRole: "name"; valueRole: "id"
                                    model: [{ id: "", name: "Actief template" }].concat(presenter.templates.all)
                                    currentIndex: { presenter.templates.kindDefaults; presenter.templates.all; return Math.max(0, indexOfValue(presenter.templates.kindTemplate(modelData.k))) }
                                    onActivated: presenter.templates.setKindTemplate(modelData.k, currentValue)
                                }
                            }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }

        // ---- preview ------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 10
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: "Voorbeeld"; color: T.text; font.pixelSize: 16; font.bold: true }
                    Item { Layout.fillWidth: true }
                    AppButton { text: "Korte tekst"; implicitHeight: 32; active: !root.longSample; onClicked: root.longSample = false }
                    AppButton { text: "Lange tekst"; implicitHeight: 32; active: root.longSample; onClicked: root.longSample = true }
                }
                Item {
                    id: holder
                    Layout.fillWidth: true; Layout.fillHeight: true
                    SlideView {
                        anchors.centerIn: parent
                        width: Math.min(holder.width, holder.height * 16 / 9); height: width * 9 / 16
                        tpl: root.draft
                        background: presenter.defaultBackground
                        reference: "Psalm 23:1-2"
                        text: root.longSample
                            ? "1 De HEERE is mijn Herder, mij zal niets ontbreken. 2 Hij doet mij nederliggen in grazige weiden; Hij voert mij zachtjes aan zeer stille wateren."
                            : "De HEERE is mijn Herder,\nmij zal niets ontbreken."
                    }
                }
            }
        }

        // ---- editor -------------------------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 340; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 8
                Text { text: "Opmaak"; color: T.text; font.pixelSize: 18; font.bold: true }
                Text {
                    visible: !root.editable; Layout.fillWidth: true; wrapMode: Text.WordWrap
                    color: T.muted; font.pixelSize: 12
                    text: "Dit is een ingebouwd template. Dupliceer het om de opmaak aan te passen."
                }
                ScrollView {
                    id: sv
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; contentWidth: availableWidth
                    enabled: root.editable
                    opacity: root.editable ? 1 : 0.55

                    ColumnLayout {
                        width: sv.availableWidth - 12; spacing: 10

                        Text { text: "Naam"; color: T.muted; font.pixelSize: 12 }
                        TextField {
                            Layout.fillWidth: true; text: root.draft.name; color: T.text; selectByMouse: true
                            background: Rectangle { color: T.card; radius: 6; border.color: parent.activeFocus ? T.accent : T.border }
                            onEditingFinished: if (text.trim() !== "") root.setKey("name", text.trim())
                        }

                        Text { text: "Lettertype"; color: T.muted; font.pixelSize: 12 }
                        ComboBox {
                            id: fontBox
                            Layout.fillWidth: true; font.pixelSize: 13
                            model: ["Standaard"].concat(Qt.fontFamilies())
                            currentIndex: Math.max(0, model.indexOf(root.draft.fontFamily))
                            onActivated: root.setKey("fontFamily", currentIndex === 0 ? "" : model[currentIndex])
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Grootte"; color: T.muted; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Text { text: Math.round(root.draft.fontSize) + " px"; color: T.text; font.pixelSize: 12 }
                        }
                        Slider { Layout.fillWidth: true; from: 40; to: 200; stepSize: 2; value: root.draft.fontSize
                            onMoved: root.setKey("fontSize", Math.round(value)) }

                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            Repeater {
                                model: [{ k: "bold", t: "Vet" }, { k: "italic", t: "Cursief" }, { k: "uppercase", t: "KAPITALEN" }, { k: "shadow", t: "Schaduw" }]
                                AppButton {
                                    Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34
                                    leftPadding: 4; rightPadding: 4; font.pixelSize: 12
                                    text: modelData.t; active: root.draft[modelData.k] === true
                                    onClicked: root.setKey(modelData.k, !(root.draft[modelData.k] === true))
                                }
                            }
                        }

                        ColorField { Layout.fillWidth: true; label: "Tekstkleur"; value: root.draft.textColor; onPicked: function (c) { root.setKey("textColor", c) } }

                        Text { text: "Uitlijning"; color: T.muted; font.pixelSize: 12 }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            Repeater {
                                model: [{ v: "left", t: "Links" }, { v: "center", t: "Midden" }, { v: "right", t: "Rechts" }]
                                AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34; text: modelData.t
                                    active: root.draft.align === modelData.v; onClicked: root.setKey("align", modelData.v) }
                            }
                        }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            Repeater {
                                model: [{ v: "top", t: "Boven" }, { v: "middle", t: "Midden" }, { v: "bottom", t: "Onder" }]
                                AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34; text: modelData.t
                                    active: root.draft.vAlign === modelData.v; onClicked: root.setKey("vAlign", modelData.v) }
                            }
                        }

                        RowLayout { Layout.fillWidth: true
                            Text { text: "Regelafstand"; color: T.muted; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Text { text: Number(root.draft.lineHeight).toFixed(2); color: T.text; font.pixelSize: 12 } }
                        Slider { Layout.fillWidth: true; from: 0.9; to: 1.6; stepSize: 0.05; value: root.draft.lineHeight
                            onMoved: root.setKey("lineHeight", Math.round(value * 100) / 100) }

                        RowLayout { Layout.fillWidth: true
                            Text { text: "Marge"; color: T.muted; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Text { text: Math.round(root.draft.margin) + " px"; color: T.text; font.pixelSize: 12 } }
                        Slider { Layout.fillWidth: true; from: 40; to: 300; stepSize: 5; value: root.draft.margin
                            onMoved: root.setKey("margin", Math.round(value)) }

                        RowLayout { Layout.fillWidth: true
                            Text { text: "Achtergrond verduisteren"; color: T.muted; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Text { text: Math.round(root.draft.dim * 100) + "%"; color: T.text; font.pixelSize: 12 } }
                        Slider { Layout.fillWidth: true; from: 0; to: 0.8; stepSize: 0.01; value: root.draft.dim
                            onMoved: root.setKey("dim", Math.round(value * 100) / 100) }

                        Rectangle { Layout.fillWidth: true; height: 1; color: T.border }
                        Text { text: "Bijbelverwijzing"; color: T.text; font.pixelSize: 14; font.bold: true }
                        ColorField { Layout.fillWidth: true; label: "Kleur"; value: root.draft.refColor; onPicked: function (c) { root.setKey("refColor", c) } }
                        RowLayout { Layout.fillWidth: true
                            Text { text: "Grootte"; color: T.muted; font.pixelSize: 12 }
                            Item { Layout.fillWidth: true }
                            Text { text: Math.round(root.draft.refSize) + " px"; color: T.text; font.pixelSize: 12 } }
                        Slider { Layout.fillWidth: true; from: 30; to: 100; stepSize: 2; value: root.draft.refSize
                            onMoved: root.setKey("refSize", Math.round(value)) }
                        AppButton { Layout.fillWidth: true; implicitHeight: 34; text: "Cursief"; active: root.draft.refItalic === true
                            onClicked: root.setKey("refItalic", !(root.draft.refItalic === true)) }
                    }
                }
            }
        }
    }
}
