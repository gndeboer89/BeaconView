import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// Right column: every slide of the current part (click one to put it live), the loop / video controls
// and the live buttons.
Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    property var video: null            // VideoEngine
    signal bibleRequested()
    signal overlaysRequested()
    readonly property bool isVideo: presenter.currentKind === "video"

    function fmt(ms) {
        var s = Math.max(0, Math.floor(ms / 1000)), m = Math.floor(s / 60)
        return (m < 10 ? "0" : "") + m + ":" + (s % 60 < 10 ? "0" : "") + (s % 60)
    }

    ColumnLayout {
        id: col
        anchors.fill: parent; anchors.margins: 14; spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text { Layout.fillWidth: true; text: presenter.currentTitle !== "" ? presenter.currentTitle : "Geen onderdeel"
                color: T.text; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
            Text { visible: !root.isVideo; text: (presenter.currentSlide + 1) + "/" + presenter.slideCount; color: T.muted; font.pixelSize: 13 }
        }

        // ---- slides -----------------------------------------------------------------------
        ListView {
            id: slides
            visible: !root.isVideo
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 5
            model: presenter.currentSlides
            ScrollBar.vertical: ScrollBar {}

            // keep the live slide in view when navigating with keys/buttons
            Connections {
                target: presenter
                function onLiveChanged() { slides.positionViewAtIndex(presenter.currentSlide, ListView.Contain) }
            }

            delegate: Rectangle {
                id: row
                readonly property bool live: index === presenter.currentSlide
                readonly property bool next: index === presenter.currentSlide + 1
                readonly property string img: presenter.currentImages[index] !== undefined ? presenter.currentImages[index] : ""
                width: slides.width - 10
                height: Math.max(52, body.implicitHeight + 18)
                radius: 8
                color: live ? T.accentSoft : (hover.hovered ? T.cardHover : T.card)
                border.color: live ? T.accent : T.border; border.width: live ? 2 : 1
                HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
                TapHandler { onTapped: presenter.goToSlide(index) }

                RowLayout {
                    id: body
                    anchors.left: parent.left; anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 10; anchors.rightMargin: 10; spacing: 10
                    Text { text: index + 1; color: row.live ? T.accent : T.muted; font.pixelSize: 13; font.bold: true
                        Layout.alignment: Qt.AlignTop; Layout.preferredWidth: 16 }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 2
                        Rectangle {   // thumbnail of an image slide
                            visible: row.img !== ""
                            Layout.fillWidth: true; Layout.preferredHeight: parent.width * 9 / 16
                            color: "black"; radius: 4
                            Image { anchors.fill: parent; source: row.img !== "" ? row.img : ""; fillMode: Image.PreserveAspectFit
                                sourceSize.height: 200; asynchronous: true }
                        }
                        Text { visible: row.img === "" || modelData !== ""; Layout.fillWidth: true; text: modelData; color: T.text; font.pixelSize: 13
                            wrapMode: Text.WordWrap; maximumLineCount: 8; elide: Text.ElideRight }
                        Text { visible: text !== ""; Layout.fillWidth: true; color: T.muted; font.pixelSize: 11
                            text: presenter.currentRefs[index] !== undefined ? presenter.currentRefs[index] : "" }
                    }
                    Text { visible: row.next; text: "volgende"; color: T.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignTop }
                }
            }
        }

        // ---- video controls --------------------------------------------------------------------
        ColumnLayout {
            visible: root.isVideo
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 10
            readonly property bool missing: root.isVideo && !presenter.fileExists(presenter.liveVideo)

            Text { visible: parent.missing; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.danger; font.pixelSize: 13
                text: "⚠ Het videobestand is niet gevonden. Staat het op een USB-stick of een andere schijf die nu niet aangesloten is?" }
            Text { visible: root.video && root.video.errorText !== ""; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.danger; font.pixelSize: 13
                text: root.video ? "Afspelen mislukt: " + root.video.errorText : "" }

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; primary: true; implicitHeight: 44
                    text: root.video && root.video.playing ? "⏸  Pauze" : "▶  Afspelen"
                    onClicked: root.video && root.video.toggle() }
                AppButton { Layout.minimumWidth: 0; implicitHeight: 44; text: "↺  Opnieuw"; onClicked: root.video && root.video.restart() }
            }
            Slider {
                Layout.fillWidth: true
                enabled: root.video && root.video.player.seekable
                from: 0; to: root.video ? Math.max(1, root.video.player.duration) : 1
                value: root.video ? root.video.player.position : 0
                onMoved: if (root.video) root.video.player.position = value
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: root.video ? root.fmt(root.video.player.position) : "00:00"; color: T.text; font.pixelSize: 13 }
                Item { Layout.fillWidth: true }
                Text { text: root.video ? root.fmt(root.video.player.duration) : "00:00"; color: T.muted; font.pixelSize: 13 }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 8
                AppButton { text: root.video && root.video.muted ? "Gedempt" : "Geluid"; implicitHeight: 34; active: root.video && root.video.muted
                    onClicked: if (root.video) root.video.muted = !root.video.muted }
                Slider { Layout.fillWidth: true; from: 0; to: 1; value: root.video ? root.video.volume : 1
                    onMoved: if (root.video) root.video.volume = value }
            }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 11
                text: (presenter.liveVideoLoop ? "Herhaalt zichzelf. " : "") + (presenter.liveVideoNext ? "Gaat na afloop door naar het volgende onderdeel. " : "")
                    + "Het zwarte scherm laat het geluid doorlopen." }
            Item { Layout.fillHeight: true }
        }

        // ---- loop ------------------------------------------------------------------------------
        RowLayout {
            visible: presenter.currentLoopSeconds > 0
            Layout.fillWidth: true; spacing: 8
            Text { Layout.fillWidth: true; color: T.muted; font.pixelSize: 12; elide: Text.ElideRight
                text: presenter.loopPaused ? "Lus gepauzeerd" : "Lus: elke " + presenter.currentLoopSeconds + " s de volgende slide" }
            AppButton { implicitHeight: 32; text: presenter.loopPaused ? "▶ Hervat" : "⏸ Pauzeer"; onClicked: presenter.loopPaused = !presenter.loopPaused }
        }

        Text {   // what comes after the last slide of this part
            Layout.fillWidth: true; elide: Text.ElideRight; color: T.muted; font.pixelSize: 12
            visible: !root.isVideo && presenter.currentSlide + 1 >= presenter.slideCount && presenter.nextText !== ""
            text: "Daarna: " + presenter.nextText.split("\n")[0]
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "‹  Vorige"; onClicked: presenter.prevSlide() }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Volgende  ›"; onClicked: presenter.nextSlide() }
        }

        AppButton { Layout.fillWidth: true; text: "→  Volgende slide"; primary: true; implicitHeight: 48; onClicked: presenter.nextSlide() }
        GridLayout {
            Layout.fillWidth: true; columns: 2; rowSpacing: 8; columnSpacing: 8
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Zwart scherm"; active: presenter.blackout; onClicked: presenter.blackout = !presenter.blackout }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Logo"; active: presenter.logo; onClicked: presenter.logo = !presenter.logo }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Timer"; active: presenter.overlays.timerShown
                onClicked: presenter.overlays.timerShown = !presenter.overlays.timerShown }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Overlay"; active: presenter.overlays.messageShown
                onClicked: presenter.overlays.messageShown ? presenter.overlays.hideMessage() : root.overlaysRequested() }
            AppButton { Layout.fillWidth: true; Layout.columnSpan: 2; Layout.minimumWidth: 0; text: "Bijbeltekst"; onClicked: root.bibleRequested() }
        }
    }
}
