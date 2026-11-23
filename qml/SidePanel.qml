import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "Theme.js" as T
import "Backgrounds.js" as Bg

Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    property int tab: 0   // 0 = media, 1 = liederen
    property int songEdit: -1

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 12

        RowLayout {
            Layout.fillWidth: true; spacing: 6
            AppButton { Layout.fillWidth: true; text: "Media"; active: root.tab === 0; onClicked: root.tab = 0 }
            AppButton { Layout.fillWidth: true; text: "Liederen"; active: root.tab === 1; onClicked: root.tab = 1 }
        }

        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: root.tab

            // ---- Media: backgrounds -------------------------------------------------
            ColumnLayout {
                spacing: 10
                Text { text: "Achtergronden"; color: T.text; font.pixelSize: 15; font.bold: true }
                Switch { id: applyAll; text: "Voor alle onderdelen gebruiken"
                    contentItem: Text { text: applyAll.text; color: T.muted; font.pixelSize: 13; leftPadding: applyAll.indicator.width + 8; verticalAlignment: Text.AlignVCenter } }
                GridView {
                    id: grid
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; cellWidth: width / 2; cellHeight: (cellWidth - 12) * 9 / 16 + 12
                    model: Bg.gradients.map(function (g) { return "g:" + g.id }).concat(presenter.mediaFiles)
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        width: grid.cellWidth; height: grid.cellHeight
                        Rectangle {
                            anchors.fill: parent; anchors.margins: 4; radius: 6; clip: true
                            color: "transparent"
                            border.color: presenter.liveBackground === modelData ? T.accent : T.border
                            border.width: presenter.liveBackground === modelData ? 2 : 1
                            SlideView { anchors.fill: parent; anchors.margins: 2; background: modelData; text: "" }
                            TapHandler { onTapped: {
                                if (applyAll.checked) {
                                    presenter.defaultBackground = modelData
                                    for (var i = 0; i < presenter.service.rowCount(); ++i) presenter.setItemBackground(i, "")
                                } else {
                                    presenter.setItemBackground(presenter.currentItem, modelData)
                                }
                            } }
                        }
                    }
                }
                AppButton { Layout.fillWidth: true; text: "Afbeelding importeren…"; onClicked: fileDialog.open() }
            }

            // ---- Liederen -------------------------------------------------------------
            ColumnLayout {
                spacing: 10
                Text { text: "Liederen (" + presenter.songs.rowCount() + ")"; color: T.text; font.pixelSize: 15; font.bold: true }
                ListView {
                    id: songList
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; spacing: 4
                    model: presenter.songs
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: songList.width - 8; height: 52; radius: 8; color: T.card
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 6; spacing: 6
                            Text { Layout.fillWidth: true; text: (number !== "" ? number + "  " : "") + title
                                color: T.text; font.pixelSize: 14; elide: Text.ElideRight }
                            AppButton { text: "+"; implicitWidth: 34; implicitHeight: 34; leftPadding: 0; rightPadding: 0
                                onClicked: presenter.addSong(index) }
                            AppButton { text: "✎"; flat: true; implicitWidth: 34; implicitHeight: 34; leftPadding: 0; rightPadding: 0
                                onClicked: { root.songEdit = index; songEditor.heading = "Lied bewerken"
                                    songEditor.openWith(title, number, lyrics) } }
                        }
                    }
                }
                AppButton { Layout.fillWidth: true; text: "Nieuw lied…"
                    onClicked: { root.songEdit = -1; songEditor.heading = "Nieuw lied"; songEditor.openWith("", "", "") } }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Kies een achtergrondafbeelding"
        nameFilters: ["Afbeeldingen (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: presenter.importMedia(selectedFile)
    }
    TextEditorDialog {
        id: songEditor
        subtitleLabel: "Liednummer (optioneel)"
        onConfirmed: function (t, s, b) {
            if (root.songEdit >= 0) presenter.songs.update(root.songEdit, t, s, b)
            else presenter.songs.add(t, s, b)
        }
    }
}
