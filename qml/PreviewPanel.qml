import QtQuick
import QtQuick.Layouts
import "Theme.js" as T

// Large live preview of the main screen.
Rectangle {
    color: T.panel; radius: 12; border.color: T.border

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 14; spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Text { text: "Hoofdscherm"; color: T.text; font.pixelSize: 16; font.bold: true }
            Text { text: presenter.outputScreen !== "" ? "(" + presenter.outputScreen + ")" : "(Beamer/LED)"; color: T.muted; font.pixelSize: 14 }
            Item { Layout.fillWidth: true }
            Rectangle {
                radius: 14; implicitHeight: 28; implicitWidth: badge.implicitWidth + 28
                color: presenter.outputOn ? "#143a22" : T.card
                Text { id: badge; anchors.centerIn: parent; font.pixelSize: 13
                    text: presenter.outputOn ? "● Live" : "○ Uit"
                    color: presenter.outputOn ? T.ok : T.muted }
            }
        }
        Item {
            id: holder
            Layout.fillWidth: true; Layout.fillHeight: true
            SlideView {
                anchors.centerIn: parent
                width: Math.min(holder.width, holder.height * 16 / 9); height: width * 9 / 16
                text: presenter.liveText; reference: presenter.liveReference; background: presenter.liveBackground
                blackout: presenter.blackout; showLogo: presenter.logo; animated: true; playVideo: true; showOverlays: true
            }
        }
    }
}
