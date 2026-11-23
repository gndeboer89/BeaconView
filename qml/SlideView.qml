import QtQuick
import "Backgrounds.js" as Bg

// One slide rendered on a virtual 1920x1080 canvas that scales to any size.
// Used for the live output window and for every preview in the control UI.
Item {
    id: root
    property string text: ""
    property string background: "g:sunset"
    property bool blackout: false
    property bool showLogo: false
    property bool animated: false
    property string shownText: text
    clip: true

    onTextChanged: { if (animated) fade.restart(); else shownText = text }

    Rectangle { anchors.fill: parent; color: "black" }

    Item {
        id: stage
        width: 1920; height: 1080
        scale: root.width / 1920
        transformOrigin: Item.TopLeft

        Rectangle {
            anchors.fill: parent
            visible: Bg.isGradient(root.background)
            gradient: Gradient {
                GradientStop { position: 0.0; color: Bg.stops(root.background)[0] }
                GradientStop { position: 0.55; color: Bg.stops(root.background)[1] }
                GradientStop { position: 1.0; color: Bg.stops(root.background)[2] }
            }
        }
        Image {
            anchors.fill: parent
            visible: !Bg.isGradient(root.background)
            source: visible ? root.background : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
        Rectangle { anchors.fill: parent; color: "#000000"; opacity: 0.22 }  // keeps text readable

        Item {
            id: textLayer
            anchors.fill: parent
            anchors.margins: 140
            Text {   // soft shadow
                anchors.fill: parent; anchors.topMargin: 5; anchors.leftMargin: 5
                text: root.shownText; color: "#99000000"
                font.pixelSize: 108; font.bold: true; lineHeight: 1.15
                fontSizeMode: Text.Fit; minimumPixelSize: 40
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
            }
            Text {
                anchors.fill: parent
                text: root.shownText; color: "white"
                font.pixelSize: 108; font.bold: true; lineHeight: 1.15
                fontSizeMode: Text.Fit; minimumPixelSize: 40
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
            }
        }

        // Built-in logo: a simple cross
        Item {
            anchors.fill: parent
            opacity: root.showLogo ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 250 } }
            Rectangle { anchors.fill: parent; color: "#101322" }
            Rectangle { width: 70; height: 380; radius: 8; color: "white"; x: 960 - 35; y: 540 - 190 }
            Rectangle { width: 250; height: 70; radius: 8; color: "white"; x: 960 - 125; y: 540 - 120 }
        }

        Rectangle {
            anchors.fill: parent; color: "black"
            opacity: root.blackout ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 200 } }
        }
    }

    SequentialAnimation {
        id: fade
        NumberAnimation { target: textLayer; property: "opacity"; to: 0; duration: 120 }
        ScriptAction { script: root.shownText = root.text }
        NumberAnimation { target: textLayer; property: "opacity"; to: 1; duration: 180 }
    }
}
