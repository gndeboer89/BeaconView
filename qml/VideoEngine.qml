import QtQuick
import QtMultimedia

// Plays the video of the live part. One player for everything: the picture goes to the output window and the
// preview through `videoHub`, the sound plays once. Controlled by the presenter and by the right-hand column.
Item {
    id: engine
    readonly property alias player: player
    property real volume: 1.0
    property bool muted: false
    property string errorText: ""
    readonly property bool playing: player.playbackState === MediaPlayer.PlayingState
    readonly property bool hasVideo: presenter.liveVideo !== ""

    function start() {
        errorText = ""
        if (player.source.toString() === presenter.liveVideo) {
            player.stop()                          // same file again: play it from the beginning
        } else {
            player.source = presenter.liveVideo
        }
        player.play()
    }
    function toggle() { if (playing) player.pause(); else player.play() }
    function restart() { player.position = 0; player.play() }

    MediaPlayer {
        id: player
        videoOutput: videoHub.source
        audioOutput: AudioOutput { volume: engine.volume; muted: engine.muted }
        loops: presenter.liveVideoLoop ? MediaPlayer.Infinite : 1
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia && presenter.liveVideoNext && !presenter.liveVideoLoop)
                presenter.nextSlide()               // continue with the next part of the service
        }
        onErrorOccurred: function (error, errorString) { engine.errorText = errorString }
    }

    Connections {
        target: presenter
        function onVideoStart() { engine.start() }
        function onSlidesChanged() {                 // the live part changed: stop and release the file when it is no longer a video
            if (presenter.liveVideo === "" && player.source.toString() !== "") { player.stop(); player.source = "" }
        }
    }
}
