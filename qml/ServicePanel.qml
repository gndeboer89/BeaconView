import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "Theme.js" as T

Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    property int editIndex: -1
    readonly property var kinds: [
        { kind: "welcome", title: "Welkom" }, { kind: "prayer", title: "Gebed" },
        { kind: "scripture", title: "Schriftlezing" }, { kind: "sermon", title: "Verkondiging" },
        { kind: "offering", title: "Collecte" }, { kind: "blessing", title: "Zegen" },
        { kind: "custom", title: "Eigen tekst…" }
    ]
    function glyph(k) { return k === "song" ? "♪" : k === "scripture" ? "▤" : k === "slides" ? "▦" : k === "loop" ? "↻" : k === "video" ? "▶" : "●" }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 14; spacing: 12

        TextField {
            Layout.fillWidth: true
            text: presenter.serviceName
            onEditingFinished: presenter.serviceName = text
            color: T.text; font.pixelSize: 20; font.bold: true; selectByMouse: true
            background: Rectangle { color: "transparent"; border.color: parent.activeFocus ? T.accent : "transparent"; radius: 6 }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            AppButton { Layout.fillWidth: true; text: "▶  Start dienst"; primary: true; implicitHeight: 46
                onClicked: presenter.startService() }
            AppButton { text: "Opslaan"; implicitHeight: 46; onClicked: presenter.save() }
        }
        Text {
            visible: presenter.lastSaved !== ""
            text: "Opgeslagen om " + presenter.lastSaved
            color: T.muted; font.pixelSize: 11
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: T.border }

        ListView {
            id: list
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 4
            model: presenter.service
            ScrollBar.vertical: ScrollBar {}

            // drag and drop: grab the handle on the left and drop the part where it should go
            readonly property real rowStep: 60          // delegate height (56) + spacing (4)
            property int dropIndex: -1                   // where the dragged part would land, -1 = nothing is dragged
            property int dragFrom: -1
            Rectangle {                                  // drop marker
                visible: list.dropIndex >= 0 && list.dropIndex !== list.dragFrom
                x: 0; width: list.width - 8; height: 3; radius: 2; color: T.accent; z: 10
                y: list.dropIndex > list.dragFrom ? (list.dropIndex + 1) * list.rowStep - 4 : list.dropIndex * list.rowStep - 3
            }

            delegate: Item {
                id: row
                width: list.width - 8; height: 56
                readonly property bool live: index === presenter.currentItem
                Rectangle {
                id: card
                width: parent.width; height: parent.height; radius: 8
                z: grip.active ? 5 : 0
                opacity: grip.active ? 0.85 : 1
                color: live ? T.accentSoft : (hover.hovered || grip.active ? T.card : "transparent")
                border.color: live || grip.active ? T.accent : "transparent"
                HoverHandler { id: hover }
                TapHandler { onTapped: presenter.selectItem(index); onDoubleTapped: openEditor(index) }
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 4; anchors.rightMargin: 6; spacing: 8
                    Item {   // drag handle
                        Layout.preferredWidth: 22; Layout.fillHeight: true
                        Text { anchors.centerIn: parent; text: "⋮⋮"; color: gripHover.hovered || grip.active ? T.accent : T.muted; font.pixelSize: 16 }
                        HoverHandler { id: gripHover; cursorShape: Qt.OpenHandCursor }
                        DragHandler {
                            id: grip
                            target: card
                            xAxis.enabled: false
                            yAxis.minimum: -index * list.rowStep
                            yAxis.maximum: (list.count - 1 - index) * list.rowStep
                            cursorShape: Qt.ClosedHandCursor
                            readonly property int dropAt: Math.max(0, Math.min(list.count - 1, Math.round((index * list.rowStep + card.y) / list.rowStep)))
                            onActiveChanged: {
                                if (active) { list.dragFrom = index; list.dropIndex = index; return }
                                const from = index, to = dropAt
                                list.dragFrom = -1; list.dropIndex = -1
                                card.y = 0
                                if (to !== from) presenter.moveItemTo(from, to)
                            }
                            onDropAtChanged: if (active) list.dropIndex = dropAt
                        }
                    }
                    Text { text: root.glyph(kind); color: live ? T.accent : T.muted; font.pixelSize: 20; Layout.preferredWidth: 22 }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 0
                        Text { text: title; color: T.text; font.pixelSize: 15; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { visible: subtitle !== ""; text: subtitle; color: T.muted; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
                    }
                    Text {   // video file is gone (USB stick, moved file)
                        visible: kind === "video" && !presenter.fileExists(presenter.itemInfo(index).video)
                        text: "⚠"; color: T.danger; font.pixelSize: 16
                    }
                    Text {   // this part has its own template; the name is shown as a tooltip
                        id: tplIcon
                        visible: templateId !== "" && presenter.templates.contains(templateId)
                        text: "▣"; color: T.accent; font.pixelSize: 16
                        HoverHandler { id: tplHover }
                        ToolTip.visible: tplHover.hovered
                        ToolTip.delay: 300
                        ToolTip.text: "Template: " + (presenter.templates.all, presenter.templates.get(templateId).name)
                    }
                    AppButton { text: "⋮"; flat: true; implicitWidth: 34; implicitHeight: 34; leftPadding: 0; rightPadding: 0
                        onClicked: { itemMenu.idx = index; itemMenu.popup() } }
                }
                }
            }
        }

        AppButton {
            Layout.fillWidth: true; text: "+  Onderdeel toevoegen"
            onClicked: addMenu.popup()
        }
    }

    // Songs keep the text editor (a blank line starts a new verse); every other part uses the WYSIWYG screen editor.
    function openEditor(i) {
        var info = presenter.itemInfo(i)
        if (info.kind === "song") {
            editIndex = i
            editor.heading = "Lied bewerken"
            editor.openWith(info.title, info.subtitle, presenter.itemText(i))
        } else if (info.kind === "video") {
            videoOptions.openFor(i)
        } else {
            slideEditor.openFor(i, false)   // text, presentation and loop parts
        }
    }
    Component.onCompleted: if (devEdit >= 0) openEditor(devEdit)   // developer aid: --edit N

    Menu {
        id: itemMenu
        property int idx: -1
        property string curTemplate: ""
        property string defaultName: ""
        onAboutToShow: { curTemplate = presenter.itemTemplate(idx); defaultName = presenter.defaultTemplateName(idx) }
        MenuItem { text: "Bewerken…"; onTriggered: root.openEditor(itemMenu.idx) }
        Menu {
            title: "Template"
            MenuItem {
                text: "Standaard (" + itemMenu.defaultName + ")"
                checkable: true; checked: itemMenu.curTemplate === "" || !presenter.templates.contains(itemMenu.curTemplate)
                onTriggered: presenter.setItemTemplate(itemMenu.idx, "")
            }
            MenuSeparator {}
            Repeater {
                model: presenter.templates.all
                MenuItem {
                    text: modelData.name
                    checkable: true; checked: itemMenu.curTemplate === modelData.id
                    onTriggered: presenter.setItemTemplate(itemMenu.idx, modelData.id)
                }
            }
        }
        MenuItem { text: "Omhoog"; enabled: itemMenu.idx > 0; onTriggered: presenter.moveItem(itemMenu.idx, -1) }
        MenuItem { text: "Omlaag"; enabled: itemMenu.idx < presenter.service.rowCount() - 1; onTriggered: presenter.moveItem(itemMenu.idx, 1) }
        MenuItem { text: "Verwijderen"; onTriggered: presenter.removeItem(itemMenu.idx) }
    }
    Menu {
        id: addMenu
        MenuItem { text: "Presentatie (PowerPoint/PDF)…"; onTriggered: presentationDialog.open() }
        MenuItem { text: "Mededelingenlus…"; onTriggered: slideEditor.openFor(presenter.addSlidesItem("loop", "Mededelingen", [], 10), true) }
        MenuItem { text: "Video…"; onTriggered: videoDialog.open() }
        MenuSeparator {}
        Repeater {
            model: root.kinds
            MenuItem {
                text: modelData.title
                onTriggered: {
                    if (modelData.kind === "custom") {
                        // add an empty part and edit it on screen; cancelling removes it again
                        slideEditor.openFor(presenter.addItem("custom", "Eigen tekst", "", ""), true)
                    } else {
                        presenter.addItem(modelData.kind, modelData.title, "", modelData.title)
                    }
                }
            }
        }
    }

    SlideEditor { id: slideEditor }
    VideoOptionsDialog { id: videoOptions; onConfirmed: function (i, t, f, l, n) { presenter.editVideo(i, t, f, l, n) } }

    FileDialog {
        id: presentationDialog
        title: "Kies een presentatie, PDF of afbeeldingen"
        fileMode: FileDialog.OpenFiles
        nameFilters: [presenter.importer.supportedFilter(), "Alle bestanden (*)"]
        onAccepted: presenter.importer.importFiles("new-slides", selectedFiles)
    }
    FileDialog {
        id: videoDialog
        title: "Kies een video"
        nameFilters: ["Video's (*.mp4 *.m4v *.mov *.webm *.mkv *.avi *.wmv)", "Alle bestanden (*)"]
        onAccepted: presenter.addVideo(selectedFile)
    }
    Connections {
        target: presenter.importer
        function onFinished(tag, images, title) {
            if (tag === "new-slides") presenter.addSlidesItem("slides", title, images, 0)
        }
    }

    TextEditorDialog {
        id: editor
        bodyLabel: "Tekst: een lege regel = nieuwe slide. Een regel als [Couplet 1] boven een deel is de naam van dat deel (komt niet op het scherm)"
        onConfirmed: function (t, s, b) {
            if (root.editIndex >= 0) presenter.editItem(root.editIndex, t, s, b)
            else presenter.addItem("custom", t, s, b)
        }
    }
}
