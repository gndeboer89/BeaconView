import QtQuick
import QtQuick.Window

// The window that is projected: borderless, covers the chosen screen completely.
// With only one screen it opens as a normal 16:9 window so the operator is not locked out.
Window {
    id: win
    readonly property bool singleScreen: Qt.application.screens.length < 2

    title: "BeaconView – Uitvoer"
    color: "black"
    visible: false
    flags: singleScreen ? Qt.Window : (Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint)

    function targetScreen() {
        var list = Qt.application.screens
        for (var i = 0; i < list.length; ++i)
            if (list[i].name === presenter.outputScreen) return list[i]
        return list.length > 1 ? list[1] : list[0]
    }
    function place() {
        var s = targetScreen()
        if (!s) return
        if (singleScreen) {
            width = 960; height = 540; x = s.virtualX + 80; y = s.virtualY + 80
        } else {
            screen = s
            x = s.virtualX; y = s.virtualY; width = s.width; height = s.height
        }
    }

    Connections {
        target: presenter
        function onOutputOnChanged() {
            if (presenter.outputOn) { win.place(); win.visible = true } else win.visible = false
        }
        function onOutputScreenChanged() { if (win.visible) win.place() }
    }
    Connections {
        target: Qt.application
        function onScreensChanged() { if (win.visible) win.place() }
    }
    onClosing: presenter.outputOn = false

    SlideView {
        anchors.fill: parent
        text: presenter.liveText
        reference: presenter.liveReference
        background: presenter.liveBackground
        blackout: presenter.blackout
        showLogo: presenter.logo
        animated: true
        playVideo: true
        showOverlays: true
        showWatermark: true
    }
    MouseArea { anchors.fill: parent; cursorShape: win.singleScreen ? Qt.ArrowCursor : Qt.BlankCursor }
}
