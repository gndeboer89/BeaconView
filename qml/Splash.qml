import QtQuick

// Start-up screen: shown at once while the program loads, so it is clear that it is starting.
// The moving bar uses an animator, which keeps moving while the rest of the program is busy loading.
Rectangle {
    id: root
    property string status: "Bezig met starten…"
    color: "#0f1218"
    border.color: "#2a3140"; border.width: 1

    Column {
        anchors.centerIn: parent; spacing: 18
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/assets/logo.png"; sourceSize.height: 120; height: 120; fillMode: Image.PreserveAspectFit
        }
        Rectangle {
            id: track
            anchors.horizontalCenter: parent.horizontalCenter
            width: 260; height: 4; radius: 2; color: "#1d2330"; clip: true
            Rectangle {
                id: bar
                width: 90; height: parent.height; radius: 2; color: "#7c5cff"; x: -width
                XAnimator on x { from: -bar.width; to: track.width; duration: 1100; loops: Animation.Infinite; running: true }
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.status; color: "#9aa3b2"; font.pixelSize: 14
        }
    }
}
