import QtQuick
import QtMultimedia

// Plays the video of the live part. One player for everything: the picture goes to the output window and the
// preview through `videoHub`, the sound plays once. Controlled by the presenter and by the right-hand column.
//
// Starting is deliberately careful (it is where video players tend to go wrong):
//  - play() is only called once the file is loaded, never straight after setting the source;
//  - a video that is the NEXT part is loaded in advance (paused on its first frame), so it starts at once;
//  - if it has not started after a few seconds it is loaded again once, then a clear message is shown;
//  - every step is written to the log file (see Instellingen) to find out what went wrong on a given computer.
Item {
    id: engine
    readonly property alias player: player
    property real volume: 1.0
    property bool muted: false
    property string errorText: ""
    property bool loading: false            // a start was requested and the video is not playing yet
    readonly property bool playing: player.playbackState === MediaPlayer.PlayingState
    readonly property bool hasVideo: presenter.liveVideo !== ""

    property bool wantPlay: false           // start requested, waiting for the file to be ready
    property bool preloading: false         // loaded in advance, to be paused on the first frame
    property int attempts: 0
    property double requestedAt: 0
    // Computed on the spot, not cached in a property: inside a status-changed handler a cached value can still be stale.
    function isReady() {
        var s = player.mediaStatus
        return s === MediaPlayer.LoadedMedia || s === MediaPlayer.BufferedMedia || s === MediaPlayer.BufferingMedia
            || s === MediaPlayer.StalledMedia || s === MediaPlayer.EndOfMedia
    }

    function log(msg) { presenter.log("video: " + msg) }
    function srcIs(url) { return player.source.toString() === url }
    function elapsed() { return Math.round(Date.now() - requestedAt) }

    function start() {
        errorText = ""; attempts = 0; wantPlay = true; preloading = false; loading = true
        requestedAt = Date.now()
        var url = presenter.liveVideo
        if (!srcIs(url)) {
            log("laden: " + url)
            if (player.playbackState !== MediaPlayer.StoppedState) player.stop()
            player.source = url
        } else if (isReady()) {
            log("al geladen, opnieuw vanaf het begin")
            player.position = 0
        }
        tryPlay()
        watchdog.restart()
    }
    function tryPlay() {
        if (!wantPlay || !isReady()) return
        wantPlay = false
        log("klaar na " + elapsed() + " ms (status " + player.mediaStatus + "), afspelen")
        player.play()
    }
    function toggle() { if (playing) player.pause(); else { wantPlay = true; tryPlay() } }
    function restart() { player.position = 0; wantPlay = true; tryPlay() }

    // When the live part is not a video: load the next video in advance, or release the file.
    function syncIdle() {
        if (presenter.liveVideo !== "") return
        wantPlay = false; loading = false; watchdog.stop()
        var up = presenter.upcomingVideo
        if (up !== "") {
            if (!srcIs(up)) {
                if (player.playbackState !== MediaPlayer.StoppedState) player.stop()
                log("voorladen: " + up)
                preloading = true
                player.source = up
            } else {                              // this file is still loaded (we just left it): park it on the first frame
                if (playing) player.pause()
                player.position = 0
                preloading = !isReady()
                if (!preloading) { player.pause(); log("geparkeerd op het eerste beeld") }
            }
        } else if (player.source.toString() !== "") {
            player.stop(); player.source = ""; preloading = false
        }
    }
    function finishPreload() {
        if (!preloading || !isReady()) return
        preloading = false
        player.pause()                       // stays on the first frame until the part goes live
        log("voorgeladen")
    }

    MediaPlayer {
        id: player
        videoOutput: videoHub.source
        audioOutput: AudioOutput { volume: engine.volume; muted: engine.muted }
        loops: presenter.liveVideoLoop ? MediaPlayer.Infinite : 1
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia && presenter.liveVideoNext && !presenter.liveVideoLoop)
                presenter.nextSlide()               // continue with the next part of the service
            engine.tryPlay()
            engine.finishPreload()
        }
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.PlayingState && engine.loading) {
                engine.loading = false
                engine.log("speelt, " + engine.elapsed() + " ms na het starten")
            }
        }
        onErrorOccurred: function (error, errorString) {
            engine.errorText = errorString
            engine.loading = false; engine.wantPlay = false
            engine.log("FOUT (" + error + "): " + errorString + " | " + source)
        }
    }

    Timer {
        id: watchdog
        interval: 4000
        onTriggered: {
            if (!engine.loading) return                 // it is playing
            engine.attempts++
            var url = presenter.liveVideo
            if (engine.attempts === 1) {
                engine.log("start duurt lang (status " + player.mediaStatus + "), bestand opnieuw laden")
                player.stop(); player.source = ""
                Qt.callLater(function () { engine.wantPlay = true; player.source = url })
                watchdog.restart()
            } else {
                engine.wantPlay = false; engine.loading = false
                engine.errorText = "De video start niet. Het bestand is mogelijk te zwaar of in een formaat dat deze computer niet afspeelt (bijvoorbeeld HEVC/H.265 of 4K). Probeer een mp4 met H.264 op 1080p."
                engine.log("start mislukt na " + engine.elapsed() + " ms (status " + player.mediaStatus + ")")
            }
        }
    }

    Connections {
        target: presenter
        function onVideoStart() { engine.start() }
        function onSlidesChanged() { engine.syncIdle() }
    }
    Component.onCompleted: syncIdle()
}
