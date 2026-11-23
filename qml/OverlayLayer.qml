import QtQuick

// Timer, clock and message bar, drawn on the 1920x1080 stage above the slide.
Item {
    id: layer
    property var tpl
    readonly property var ov: presenter.overlays
    readonly property string fam: tpl.fontFamily !== "" ? tpl.fontFamily : Qt.application.font.family
    property date now: new Date()

    Timer { interval: 1000; running: layer.ov.clockShown; repeat: true; triggeredOnStart: true; onTriggered: layer.now = new Date() }

    // ---- clock (top left) -----------------------------------------------------------
    Rectangle {
        x: 60; y: 50; height: 104; radius: 52; width: clockText.implicitWidth + 80
        color: "#c0101322"
        opacity: layer.ov.clockShown ? 1 : 0; visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        Text { id: clockText; anchors.centerIn: parent; text: Qt.formatTime(layer.now, "HH:mm"); color: "white"
            font.family: layer.fam; font.pixelSize: 66; font.bold: true }
    }

    // ---- timer ----------------------------------------------------------------------
    Item {
        id: timerBox
        anchors.fill: parent
        opacity: layer.ov.timerShown ? 1 : 0; visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 250 } }
        readonly property color valueColor: layer.ov.timerFinished ? "#ff6b6b" : "white"

        Rectangle {   // big, centred
            visible: !layer.ov.timerCorner
            anchors.centerIn: parent
            width: Math.max(1000, bigTime.implicitWidth + 160); height: bigCol.implicitHeight + 110; radius: 48
            color: "#d9101322"
            Column {
                id: bigCol
                anchors.centerIn: parent; spacing: 10
                Text { visible: layer.ov.timerLabel !== ""; anchors.horizontalCenter: parent.horizontalCenter
                    text: layer.ov.timerLabel; color: "#e6e6f2"; font.family: layer.fam; font.pixelSize: 76 }
                Text { id: bigTime; anchors.horizontalCenter: parent.horizontalCenter
                    text: layer.ov.timerText; color: timerBox.valueColor
                    font.family: layer.fam; font.pixelSize: 330; font.bold: true }
            }
        }
        Rectangle {   // small, top right
            visible: layer.ov.timerCorner
            anchors.right: parent.right; anchors.rightMargin: 60; y: 50
            height: 104; radius: 52; width: cornerTime.implicitWidth + 80
            color: "#c0101322"
            Text { id: cornerTime; anchors.centerIn: parent; text: layer.ov.timerText; color: timerBox.valueColor
                font.family: layer.fam; font.pixelSize: 66; font.bold: true }
        }
    }

    // ---- message bar (bottom) ---------------------------------------------------------
    Rectangle {
        id: bar
        x: 80; width: parent.width - 160
        height: msg.implicitHeight + 70; radius: 20
        color: "#e6101322"
        opacity: layer.ov.messageShown ? 1 : 0; visible: opacity > 0
        y: parent.height - 70 - height + (layer.ov.messageShown ? 0 : 40)
        Behavior on opacity { NumberAnimation { duration: 250 } }
        Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
        Rectangle { width: 16; height: parent.height - 36; x: 22; anchors.verticalCenter: parent.verticalCenter; radius: 8; color: "#8b5cf6" }
        Text {
            id: msg
            anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 70; anchors.rightMargin: 50
            text: layer.ov.messageText; color: "white"; wrapMode: Text.WordWrap
            font.family: layer.fam; font.pixelSize: 62
        }
    }
}
