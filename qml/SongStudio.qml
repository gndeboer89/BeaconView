import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// BeaconView Liederen: the song page in a window of its own.
ApplicationWindow {
    id: win
    width: 1280; height: 800; minimumWidth: 1100; minimumHeight: 700
    visible: true
    title: "BeaconView Liederen"
    color: T.bg
    palette {
        window: T.panel; windowText: T.text; base: T.card; text: T.text
        button: T.card; buttonText: T.text; highlight: T.accent; highlightedText: "white"
        mid: T.border; dark: T.border; light: T.cardHover; placeholderText: T.muted; toolTipBase: T.card; toolTipText: T.text
    }

    SongsPage { id: songsPage; anchors.fill: parent }
    Component.onCompleted: if (devSong >= 0) songsPage.openSong(devSong === 99 ? -1 : devSong)   // developer aid: --edit-song N

    // closing with changes that are not saved asks first
    property bool closeConfirmed: false
    onClosing: function (close) {
        if (closeConfirmed || !songsPage.changed) return
        close.accepted = false
        closeDialog.open()
    }
    Dialog {
        id: closeDialog
        modal: true
        x: Math.round((win.width - width) / 2); y: Math.round((win.height - height) / 2)
        width: 480; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Wijzigingen opslaan?"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: "Het lied dat je bewerkt heeft wijzigingen die nog niet zijn opgeslagen." }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: closeDialog.close() }
                AppButton { text: "Weggooien"; onClicked: { closeDialog.close(); win.closeConfirmed = true; win.close() } }
                AppButton { text: "Opslaan"; primary: true
                    onClicked: { closeDialog.close(); if (songsPage.save()) { win.closeConfirmed = true; win.close() } } }
            }
        }
    }
}
