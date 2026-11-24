import QtQuick

// Subtitle screen: the live text on one flat colour, to key out in a video mixer or streaming software.
// Only the text of the slide; no background, logo, timer or overlays.
Item {
    id: root
    property string chroma: "#00b140"
    property string textColor: "#ffffff"
    property real scale1: 1.0
    property string position: "bottom"      // bottom, middle, top
    property string sampleText: ""          // preview in the settings page when nothing is live
    readonly property real u: height / 1080
    readonly property string family: presenter.liveTemplate.fontFamily ? presenter.liveTemplate.fontFamily : ""
    readonly property string shown: (presenter.blackout || presenter.textHidden || presenter.liveImage !== "") ? "" : (presenter.liveText !== "" ? presenter.liveText : sampleText)
    clip: true

    Rectangle { anchors.fill: parent; color: root.chroma }

    Text {
        id: sub
        width: parent.width * 0.84
        anchors.horizontalCenter: parent.horizontalCenter
        y: root.position === "top" ? 70 * root.u
         : root.position === "middle" ? (parent.height - height) / 2
         : parent.height - height - 90 * root.u
        text: root.shown
        opacity: root.shown !== "" ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
        color: root.textColor; font.family: root.family; font.bold: true
        font.pixelSize: 62 * root.u * root.scale1
        wrapMode: Text.WordWrap; horizontalAlignment: Text.AlignHCenter
        style: Text.Outline; styleColor: "#000000"      // outline so the text stays readable on any key colour
        lineHeight: 1.1
    }
}
