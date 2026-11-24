import QtQuick
import "Theme.js" as T

// Stage screen: for the people on stage. The current text large, the next text below it, a clock and the timer.
// Everything scales with the height, so it fits any screen and the small preview.
Item {
    id: root
    property real scale1: 1.0          // text size factor from the settings
    property bool showNext: true
    property bool showClock: true
    property bool showTimer: true
    readonly property real u: height / 1080
    readonly property string family: presenter.liveTemplate.fontFamily ? presenter.liveTemplate.fontFamily : ""
    readonly property var labels: presenter.currentLabels
    readonly property string curLabel: labels[presenter.currentSlide] !== undefined ? labels[presenter.currentSlide] : ""
    readonly property string nextLabel: labels[presenter.currentSlide + 1] !== undefined ? labels[presenter.currentSlide + 1] : ""
    readonly property bool imageSlide: presenter.liveImage !== ""
    clip: true

    Rectangle { anchors.fill: parent; color: "#000000" }

    // top bar: title, state, clock, timer
    Item {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        anchors.margins: 40 * root.u; height: 60 * root.u
        Text {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
            width: parent.width * 0.5; elide: Text.ElideRight
            text: presenter.currentTitle + (presenter.slideCount > 1 ? "   " + (presenter.currentSlide + 1) + "/" + presenter.slideCount : "")
            color: "#9aa3b2"; font.pixelSize: 36 * root.u
        }
        Rectangle {
            visible: presenter.blackout || presenter.textHidden
            anchors.centerIn: parent; radius: 8 * root.u; color: "#b3261e"
            width: stateText.implicitWidth + 40 * root.u; height: 50 * root.u
            Text { id: stateText; anchors.centerIn: parent; color: "white"; font.pixelSize: 30 * root.u; font.bold: true
                text: presenter.blackout ? "ZWART SCHERM" : "TEKST VERBORGEN" }
        }
        Row {
            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; spacing: 40 * root.u
            Text {
                visible: root.showTimer && presenter.overlays.timerShown
                text: presenter.overlays.timerText; font.pixelSize: 48 * root.u; font.bold: true
                color: presenter.overlays.timerFinished ? "#ff5a4f" : "#ffe45c"
            }
            Text {
                id: clock
                visible: root.showClock
                font.pixelSize: 48 * root.u; color: "#ffffff"
                text: Qt.formatTime(new Date(), "HH:mm")
                Timer { interval: 1000; running: clock.visible; repeat: true; triggeredOnStart: true
                    onTriggered: clock.text = Qt.formatTime(new Date(), "HH:mm") }
            }
        }
    }

    // current text
    Item {
        id: curArea
        anchors.left: parent.left; anchors.right: parent.right; anchors.top: bar.bottom
        anchors.leftMargin: 80 * root.u; anchors.rightMargin: 80 * root.u; anchors.topMargin: 10 * root.u
        height: root.showNext ? parent.height * 0.5 : parent.height * 0.8
        Text {
            visible: root.curLabel !== ""
            anchors.left: parent.left; anchors.top: parent.top
            text: root.curLabel; color: "#7c5cff"; font.pixelSize: 34 * root.u; font.bold: true
        }
        Image {
            visible: root.imageSlide
            anchors.fill: parent; anchors.topMargin: 50 * root.u
            source: root.imageSlide ? presenter.liveImage : ""; fillMode: Image.PreserveAspectFit; asynchronous: true
        }
        Text {
            visible: !root.imageSlide
            anchors.fill: parent; anchors.topMargin: root.curLabel !== "" ? 50 * root.u : 0
            text: presenter.liveText; color: "#ffffff"; font.family: root.family; font.bold: true
            font.pixelSize: 84 * root.u * root.scale1; minimumPixelSize: 20 * root.u
            fontSizeMode: Text.Fit; wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
    }

    // next text
    Item {
        visible: root.showNext
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.top: curArea.bottom
        anchors.leftMargin: 80 * root.u; anchors.rightMargin: 80 * root.u; anchors.bottomMargin: 40 * root.u
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top; height: Math.max(1, 2 * root.u); color: "#2a3140" }
        Text {
            anchors.left: parent.left; anchors.top: parent.top; anchors.topMargin: 14 * root.u
            text: "VOLGENDE" + (root.nextLabel !== "" ? "  ·  " + root.nextLabel : ""); color: "#9aa3b2"; font.pixelSize: 28 * root.u; font.bold: true
        }
        Text {
            anchors.fill: parent; anchors.topMargin: 60 * root.u
            text: presenter.nextText; color: "#c9d1de"; font.family: root.family
            font.pixelSize: 58 * root.u * root.scale1; minimumPixelSize: 16 * root.u
            fontSizeMode: Text.Fit; wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
        }
    }
}
