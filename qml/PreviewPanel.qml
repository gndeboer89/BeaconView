import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

ColumnLayout {
    spacing: 12

    Rectangle {
        Layout.fillWidth: true; Layout.fillHeight: true
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
                    text: presenter.liveText; background: presenter.liveBackground
                    blackout: presenter.blackout; showLogo: presenter.logo; animated: true
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true; Layout.preferredHeight: 200
        color: T.panel; radius: 12; border.color: T.border
        RowLayout {
            anchors.fill: parent; anchors.margins: 14; spacing: 12
            ColumnLayout {
                spacing: 6
                Text { text: "Huidige slide (" + (presenter.currentSlide + 1) + "/" + presenter.slideCount + ")"; color: T.muted; font.pixelSize: 13 }
                Rectangle {
                    Layout.preferredWidth: 252; Layout.preferredHeight: 144; color: "transparent"
                    border.color: T.accent; border.width: 2; radius: 6
                    SlideView { anchors.fill: parent; anchors.margins: 3; text: presenter.liveText; background: presenter.liveBackground }
                }
            }
            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter; Layout.topMargin: 22; spacing: 8
                AppButton { text: "‹"; implicitWidth: 40; leftPadding: 0; rightPadding: 0; onClicked: presenter.prevSlide() }
                AppButton { text: "›"; implicitWidth: 40; leftPadding: 0; rightPadding: 0; onClicked: presenter.nextSlide() }
            }
            ColumnLayout {
                spacing: 6
                Text { text: "Volgende slide"; color: T.muted; font.pixelSize: 13 }
                Rectangle {
                    Layout.preferredWidth: 252; Layout.preferredHeight: 144; color: "transparent"
                    border.color: T.border; radius: 6
                    SlideView { anchors.fill: parent; anchors.margins: 3; text: presenter.nextText; background: presenter.nextBackground }
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    RowLayout {
        Layout.fillWidth: true; spacing: 10
        AppButton { text: "→  Volgende slide"; primary: true; implicitHeight: 48; onClicked: presenter.nextSlide() }
        AppButton { text: "Zwart scherm"; implicitHeight: 48; active: presenter.blackout; onClicked: presenter.blackout = !presenter.blackout }
        AppButton { text: "Logo"; implicitHeight: 48; active: presenter.logo; onClicked: presenter.logo = !presenter.logo }
        Item { Layout.fillWidth: true }
    }
}
