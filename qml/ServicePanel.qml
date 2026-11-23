import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
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
    function glyph(k) { return k === "song" ? "♪" : k === "scripture" ? "▤" : "●" }

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
            delegate: Rectangle {
                width: list.width - 8; height: 56; radius: 8
                readonly property bool live: index === presenter.currentItem
                color: live ? T.accentSoft : (hover.hovered ? T.card : "transparent")
                border.color: live ? T.accent : "transparent"
                HoverHandler { id: hover }
                TapHandler { onTapped: presenter.selectItem(index); onDoubleTapped: openEditor(index) }
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 6; spacing: 12
                    Text { text: root.glyph(kind); color: live ? T.accent : T.muted; font.pixelSize: 20; Layout.preferredWidth: 22 }
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 0
                        Text { text: title; color: T.text; font.pixelSize: 15; elide: Text.ElideRight; Layout.fillWidth: true }
                        Text { visible: subtitle !== ""; text: subtitle; color: T.muted; font.pixelSize: 13; elide: Text.ElideRight; Layout.fillWidth: true }
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
        } else {
            slideEditor.openFor(i, false)
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

    TextEditorDialog {
        id: editor
        onConfirmed: function (t, s, b) {
            if (root.editIndex >= 0) presenter.editItem(root.editIndex, t, s, b)
            else presenter.addItem("custom", t, s, b)
        }
    }
}
