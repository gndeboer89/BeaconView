import QtQuick
import QtQuick.Window

// One extra output window (stage screen or subtitle screen). On a chosen monitor it is borderless and fills it;
// without a monitor it is a normal window that can be moved, resized or captured.
Window {
    id: win
    required property int index
    required property string name
    required property string kind
    required property string screenName
    required property bool on
    required property string chromaColor
    required property string textColor
    required property real scale
    required property string position
    required property bool showNext
    required property bool showClock
    required property bool showTimer

    readonly property var target: {
        var list = Qt.application.screens
        for (var i = 0; i < list.length; ++i) if (screenName !== "" && list[i].name === screenName) return list[i]
        return null
    }
    title: "BeaconView – " + name          // the window title is how a capture program finds this window
    color: "black"
    visible: on
    flags: target ? (Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint) : Qt.Window

    function place() {
        if (target) {
            screen = target
            x = target.virtualX; y = target.virtualY; width = target.width; height = target.height
        } else {
            var s = Qt.application.screens[0]
            width = 960; height = 540
            if (s) { x = s.virtualX + 120 + 40 * index; y = s.virtualY + 120 + 40 * index }
        }
    }
    onVisibleChanged: if (visible) place()
    onTargetChanged: if (visible) place()
    onClosing: presenter.extraScreens.set(index, "on", false)

    Loader {
        anchors.fill: parent
        sourceComponent: win.kind === "stage" ? stage : subtitles
    }
    Component { id: stage; StageView { scale1: win.scale; showNext: win.showNext; showClock: win.showClock; showTimer: win.showTimer } }
    Component { id: subtitles; SubtitleView { chroma: win.chromaColor; textColor: win.textColor; scale1: win.scale; position: win.position } }
    MouseArea { anchors.fill: parent; cursorShape: win.target ? Qt.BlankCursor : Qt.ArrowCursor }
}
