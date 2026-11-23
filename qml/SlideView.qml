import QtQuick
import QtMultimedia
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
    property bool showWatermark: false  // small church logo in a corner (live output + main preview)
    property bool showOverlays: false   // timer / clock / message bar (live output + main preview only)
    // Look of the text; defaults to the active template, the template editor overrides it.
    property var tpl: presenter.templates.active
    // The style that is actually drawn. It changes together with the text (during the cross-fade),
    // so switching to a slide with another template does not flash the new style on the old text.
    property var shownTpl: tpl
    onTplChanged: Qt.callLater(function () { if (!fade.running) shownTpl = tpl })
    // Video backgrounds only play where this is true (live output + main preview);
    // small thumbnails show a placeholder so we never run many decoders at once.
    property bool playVideo: false
    readonly property bool isVideo: Bg.isVideo(background)
    property bool videoFailed: false
    readonly property string gradientId: isVideo ? "g:graphite" : background
    onBackgroundChanged: videoFailed = false
    property string reference: ""
    property string shownText: text
    property string shownRef: reference
    clip: true

    onTextChanged: { if (animated) fade.restart(); else shownText = text }
    onReferenceChanged: { if (animated) fade.restart(); else shownRef = reference }

    Rectangle { anchors.fill: parent; color: "black" }

    Item {
        id: stage
        width: 1920; height: 1080
        scale: root.width / 1920
        transformOrigin: Item.TopLeft

        Rectangle {
            anchors.fill: parent
            visible: Bg.isGradient(root.gradientId)
            gradient: Gradient {
                GradientStop { position: 0.0; color: Bg.stops(root.gradientId)[0] }
                GradientStop { position: 0.55; color: Bg.stops(root.gradientId)[1] }
                GradientStop { position: 1.0; color: Bg.stops(root.gradientId)[2] }
            }
        }
        Image {
            anchors.fill: parent
            visible: !Bg.isGradient(root.background) && !root.isVideo
            source: visible ? root.background : ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
        Text {   // placeholder for video backgrounds in thumbnails
            anchors.centerIn: parent
            visible: root.isVideo && (!root.playVideo || root.videoFailed) && root.text === ""
            text: root.videoFailed ? "⚠" : "▶"; color: "#66ffffff"; font.pixelSize: 260
        }
        Loader {   // silent, looping video
            anchors.fill: parent
            active: root.isVideo && root.playVideo && !root.videoFailed
            sourceComponent: Item {
                MediaPlayer {
                    id: player
                    source: root.background
                    videoOutput: video
                    loops: MediaPlayer.Infinite
                    onSourceChanged: play()
                    onErrorOccurred: function (error, errorString) {
                        console.warn("Video kan niet worden afgespeeld:", errorString)
                        root.videoFailed = true
                    }
                    Component.onCompleted: play()
                }
                VideoOutput { id: video; anchors.fill: parent; fillMode: VideoOutput.PreserveAspectCrop }
            }
        }
        Rectangle { anchors.fill: parent; color: "#000000"; opacity: Math.min(0.9, root.shownTpl.dim + (root.isVideo ? 0.13 : 0)) }  // keeps text readable

        Item {
            id: textLayer
            anchors.fill: parent
            Item {
                anchors.fill: parent
                anchors.margins: root.shownTpl.margin
                anchors.bottomMargin: root.shownRef !== "" ? Math.max(root.shownTpl.margin, 250) : root.shownTpl.margin
                Repeater {
                    model: root.shownTpl.shadow ? 2 : 1     // first a soft shadow copy, then the text itself
                    Text {
                        readonly property bool isShadow: root.shownTpl.shadow && index === 0
                        x: isShadow ? 5 : 0; y: isShadow ? 5 : 0
                        width: parent.width; height: parent.height
                        text: root.shownText
                        color: isShadow ? "#99000000" : root.shownTpl.textColor
                        font.family: root.shownTpl.fontFamily !== "" ? root.shownTpl.fontFamily : Qt.application.font.family
                        font.pixelSize: root.shownTpl.fontSize
                        font.bold: root.shownTpl.bold; font.italic: root.shownTpl.italic
                        font.capitalization: root.shownTpl.uppercase ? Font.AllUppercase : Font.MixedCase
                        lineHeight: root.shownTpl.lineHeight
                        fontSizeMode: Text.Fit; minimumPixelSize: Math.min(40, root.shownTpl.fontSize)
                        wrapMode: Text.WordWrap
                        horizontalAlignment: root.shownTpl.align === "left" ? Text.AlignLeft : root.shownTpl.align === "right" ? Text.AlignRight : Text.AlignHCenter
                        verticalAlignment: root.shownTpl.vAlign === "top" ? Text.AlignTop : root.shownTpl.vAlign === "bottom" ? Text.AlignBottom : Text.AlignVCenter
                    }
                }
            }
            Text {   // reference, e.g. "Psalm 23:1-2"
                visible: root.shownRef !== ""
                anchors.horizontalCenter: parent.horizontalCenter
                y: 1080 - 100 - height
                text: root.shownRef; color: root.shownTpl.refColor
                font.family: root.shownTpl.fontFamily !== "" ? root.shownTpl.fontFamily : Qt.application.font.family
                font.pixelSize: root.shownTpl.refSize; font.italic: root.shownTpl.refItalic
                style: Text.Raised; styleColor: "#99000000"
            }
        }

        // Watermark: the church logo in a corner (hidden behind the logo screen and the black screen)
        Image {
            readonly property bool on: root.showWatermark && presenter.branding.watermark && presenter.branding.source !== ""
            readonly property int corner: presenter.branding.corner
            visible: on
            source: on ? presenter.branding.source : ""
            width: 1920 * presenter.branding.watermarkSize; height: width
            x: (corner % 2 === 0) ? 50 : 1920 - 50 - width
            y: corner < 2 ? 50 : 1080 - 50 - height
            horizontalAlignment: corner % 2 === 0 ? Image.AlignLeft : Image.AlignRight
            verticalAlignment: corner < 2 ? Image.AlignTop : Image.AlignBottom
            fillMode: Image.PreserveAspectFit
            sourceSize.width: 512; sourceSize.height: 512
            opacity: 0.9; asynchronous: true
        }

        // Logo screen: own logo (or the built-in cross) centred on a solid colour
        Item {
            anchors.fill: parent
            opacity: root.showLogo ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 250 } }
            Rectangle { anchors.fill: parent; color: presenter.branding.bgColor }
            Image {
                visible: presenter.branding.source !== ""
                anchors.centerIn: parent
                width: 1920 * presenter.branding.scale; height: 1080 * presenter.branding.scale
                source: visible ? presenter.branding.source : ""
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 1920; sourceSize.height: 1080
                asynchronous: true
            }
            Item {   // built-in cross
                visible: presenter.branding.source === ""
                anchors.centerIn: parent
                scale: presenter.branding.scale / 0.6
                Rectangle { width: 70; height: 380; radius: 8; color: "white"; x: -35; y: -190 }
                Rectangle { width: 250; height: 70; radius: 8; color: "white"; x: -125; y: -120 }
            }
        }

        Loader {
            anchors.fill: parent
            active: root.showOverlays
            sourceComponent: OverlayLayer { tpl: root.tpl }
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
        ScriptAction { script: { root.shownText = root.text; root.shownRef = root.reference; root.shownTpl = root.tpl } }
        NumberAnimation { target: textLayer; property: "opacity"; to: 1; duration: 180 }
    }
}
