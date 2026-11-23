import QtQuick
import "Theme.js" as T

Item {
    property string title: ""
    property string note: "Komt in een volgende versie."
    Column {
        anchors.centerIn: parent; spacing: 8
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: title; color: T.text; font.pixelSize: 22; font.bold: true }
        Text { anchors.horizontalCenter: parent.horizontalCenter; text: note; color: T.muted; font.pixelSize: 14 }
    }
}
