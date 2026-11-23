import QtQuick
import QtQuick.Controls.Basic
import "Theme.js" as T

Button {
    id: c
    property bool primary: false
    property bool active: false
    implicitHeight: 42
    leftPadding: 16; rightPadding: 16
    font.pixelSize: 14
    hoverEnabled: true

    contentItem: Text {
        text: c.text; font: c.font
        color: c.enabled ? T.text : T.muted
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 8
        color: c.primary ? (c.down ? T.accentDark : T.accent)
             : c.active ? T.accentSoft
             : c.flat ? (c.hovered ? T.card : "transparent")
             : (c.hovered ? T.cardHover : T.card)
        border.color: c.active ? T.accent : (c.flat || c.primary ? "transparent" : T.border)
    }
}
