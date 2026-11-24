import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import "Theme.js" as T
import "Backgrounds.js" as Bg

// Media library: built-in backgrounds plus the pictures and videos you imported.
Item {
    id: root
    property string filter: "all"        // all | background | image | video
    property string selected: ""
    property int rev: 0                  // bumped when the library or the service changes (refreshes info and usage)
    readonly property var entries: build()
    property var current: null            // the selected entry; kept in sync by refreshCurrent() (a plain binding would loop)
    readonly property var info: { rev; return (current && !current.builtin) ? presenter.mediaInfo(current.url) : ({}) }
    readonly property var usage: { rev; return current ? presenter.mediaUsage(current.url) : ({ parts: 0, isDefault: false }) }

    function build() {
        var list = []
        if (filter === "all" || filter === "background")
            for (var i = 0; i < Bg.gradients.length; ++i)
                list.push({ url: "g:" + Bg.gradients[i].id, name: Bg.gradients[i].name, type: "background", builtin: true })
        var m = presenter.mediaFiles
        for (var k = 0; k < m.length; ++k) {
            var t = Bg.isVideo(m[k]) ? "video" : "image"
            if (filter === "all" || filter === t)
                list.push({ url: m[k], name: decodeURIComponent(m[k].split("/").pop()), type: t, builtin: false })
        }
        return list
    }
    function find(url) {
        for (var i = 0; i < entries.length; ++i) if (entries[i].url === url) return entries[i]
        return null
    }
    function refreshCurrent() { current = find(selected) }
    onSelectedChanged: refreshCurrent()
    onEntriesChanged: {
        if (!find(selected)) selected = entries.length > 0 ? entries[0].url : ""
        refreshCurrent()
    }
    Component.onCompleted: { if (selected === "" && entries.length > 0) selected = entries[0].url; refreshCurrent() }

    Connections {
        target: presenter
        function onLiveChanged() { root.rev++ }
        function onMediaFilesChanged() { root.rev++ }
    }

    RowLayout {
        anchors.fill: parent; spacing: 12

        // ---- library -------------------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 10
                RowLayout {
                    Layout.fillWidth: true; spacing: 6
                    Text { text: "Media"; color: T.text; font.pixelSize: 20; font.bold: true; Layout.rightMargin: 12 }
                    Repeater {
                        model: [{ f: "all", t: "Alles" }, { f: "background", t: "Achtergronden" }, { f: "image", t: "Afbeeldingen" }, { f: "video", t: "Video's" }]
                        AppButton { text: modelData.t; implicitHeight: 34; leftPadding: 12; rightPadding: 12; active: root.filter === modelData.f
                            onClicked: root.filter = modelData.f }
                    }
                    Item { Layout.fillWidth: true }
                    AppButton { text: "Importeren…"; primary: true; implicitHeight: 34; onClicked: importDialog.open() }
                }

                GridView {
                    id: grid
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; cellWidth: 214; cellHeight: 150
                    model: root.entries
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        id: cell
                        width: grid.cellWidth; height: grid.cellHeight
                        readonly property bool sel: root.selected === modelData.url
                        readonly property string thumb: { videoThumbs.revision; return modelData.type === "video" ? videoThumbs.thumbnail(modelData.url) : "" }
                        Rectangle {
                            anchors.fill: parent; anchors.margins: 6; radius: 8; color: T.card
                            border.color: cell.sel ? T.accent : T.border; border.width: cell.sel ? 2 : 1
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: 6; spacing: 4
                                Item {
                                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                                    SlideView { anchors.fill: parent; visible: modelData.type === "background"; background: modelData.url; text: "" }
                                    Image { anchors.fill: parent; visible: modelData.type === "image"; source: visible ? modelData.url : ""
                                        fillMode: Image.PreserveAspectCrop; sourceSize.width: 400; asynchronous: true }
                                    Image { anchors.fill: parent; visible: modelData.type === "video" && cell.thumb !== ""; source: cell.thumb
                                        fillMode: Image.PreserveAspectCrop; asynchronous: true }
                                    Rectangle { anchors.fill: parent; visible: modelData.type === "video" && cell.thumb === ""; color: "#0f1117" }
                                    Text { anchors.centerIn: parent; visible: modelData.type === "video"; text: "▶"; font.pixelSize: 26
                                        color: cell.thumb !== "" ? "#ccffffff" : "#66ffffff"; style: Text.Outline; styleColor: "#66000000" }
                                }
                                Text { Layout.fillWidth: true; text: modelData.name; color: T.text; font.pixelSize: 12; elide: Text.ElideMiddle }
                            }
                            HoverHandler { cursorShape: Qt.PointingHandCursor }
                            TapHandler { onTapped: root.selected = modelData.url }
                        }
                    }
                }
                Text {
                    visible: root.entries.length === 0
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                    text: "Hier staat nog niets. Klik op Importeren… om afbeeldingen of video's toe te voegen."
                }
            }
        }

        // ---- detail --------------------------------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 340; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 14; spacing: 8
                visible: root.current !== null

                Text { text: "Voorbeeld"; color: T.muted; font.pixelSize: 12 }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 312 * 9 / 16
                    color: "transparent"; border.color: T.border; radius: 6
                    SlideView {
                        anchors.fill: parent; anchors.margins: 2
                        background: root.current ? root.current.url : "g:sunset"
                        playVideo: !!root.current && root.current.type === "video"
                        text: "Voorbeeld"
                    }
                }
                Text { Layout.fillWidth: true; text: root.current ? root.current.name : ""; color: T.text; font.pixelSize: 16; font.bold: true; elide: Text.ElideMiddle }
                Text {
                    Layout.fillWidth: true; color: T.muted; font.pixelSize: 12; wrapMode: Text.WordWrap
                    text: !root.current ? "" : root.current.builtin ? "Ingebouwde achtergrond"
                        : (root.current.type === "video" ? "Video" : "Afbeelding") + " · " + (root.info.sizeText || "")
                          + (root.info.exists === false ? " · ⚠ bestand ontbreekt" : "")
                }
                Text {
                    Layout.fillWidth: true; font.pixelSize: 12; wrapMode: Text.WordWrap
                    color: root.usage.parts > 0 || root.usage.isDefault ? T.ok : T.muted
                    text: (root.usage.isDefault ? "Standaardachtergrond. " : "")
                        + (root.usage.parts > 0 ? "In gebruik bij " + root.usage.parts + (root.usage.parts === 1 ? " onderdeel." : " onderdelen.")
                                                : (root.usage.isDefault ? "" : "Niet in gebruik."))
                }

                Item { Layout.fillHeight: true }

                AppButton { Layout.fillWidth: true; text: "Gebruik voor het huidige onderdeel"; implicitHeight: 40
                    onClicked: { presenter.setItemBackground(presenter.currentItem, root.current.url); root.rev++ } }
                AppButton { Layout.fillWidth: true; text: "Als standaardachtergrond"; implicitHeight: 40
                    onClicked: { presenter.defaultBackground = root.current.url; root.rev++ } }
                AppButton { visible: !!root.current && root.current.type === "video"; Layout.fillWidth: true; text: "Toevoegen als video-onderdeel"; implicitHeight: 40
                    onClicked: { presenter.addVideo(root.current.url); root.rev++ } }
                AppButton { visible: !!root.current && !root.current.builtin; Layout.fillWidth: true; text: "Verwijderen…"; implicitHeight: 40
                    onClicked: removeDialog.open() }
            }
        }
    }

    FileDialog {
        id: importDialog
        title: "Kies afbeeldingen of video's"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Afbeeldingen en video's (*.png *.jpg *.jpeg *.webp *.bmp *.mp4 *.m4v *.mov *.webm *.mkv)", "Alle bestanden (*)"]
        onAccepted: { for (var i = 0; i < selectedFiles.length; ++i) presenter.importMedia(selectedFiles[i]) }
    }

    Dialog {
        id: removeDialog
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Verwijderen uit de bibliotheek?"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text {
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: (root.current ? root.current.name : "") + " wordt verwijderd uit de app. Je originele bestand blijft staan."
                    + (root.usage.parts > 0 || root.usage.isDefault
                       ? "\n\nHet wordt nu gebruikt; die onderdelen vallen terug op de standaardachtergrond"
                         + (!!root.current && root.current.type === "video" ? " en een video-onderdeel met dit bestand kan dan niet meer afspelen." : ".") : "")
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: removeDialog.close() }
                AppButton { text: "Verwijderen"; primary: true
                    onClicked: { var u = root.current.url; videoThumbs.forget(u); presenter.removeMedia(u); removeDialog.close() } }
            }
        }
    }
}
