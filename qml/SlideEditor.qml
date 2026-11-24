import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Window
import QtQuick.Dialogs
import "Theme.js" as T

// WYSIWYG screen editor for every part except songs: the text is typed directly on the slide,
// with the template, font and background the audience will see.
Dialog {
    id: dlg
    property int itemIndex: -1
    property bool isNew: false          // a freshly added part: cancelling removes it again
    property bool saved: false
    property string kind: ""
    property int cur: 0
    property bool loading: false
    property bool structureChanged: false   // slides added/removed/moved: bible references no longer line up
    property bool hasRefs: false
    property string draftTemplate: ""       // "" = the default for this kind of part
    property string draftBackground: ""     // "" = the default background
    property int draftLoop: 0               // > 0: slides advance by themselves every N seconds
    readonly property string curImage: cur < slidesModel.count ? (slidesModel.get(cur).image || "") : ""

    readonly property var tpl: resolve(draftTemplate)
    readonly property string bg: draftBackground !== "" ? draftBackground : presenter.defaultBackground
    readonly property string defaultName: {
        presenter.templates.all; presenter.templates.kindDefaults; presenter.templates.activeId
        var k = presenter.templates.kindTemplate(kind)
        return (k !== "" ? presenter.templates.get(k) : presenter.templates.active).name
    }

    function resolve(id) {
        if (id !== "" && presenter.templates.contains(id)) return presenter.templates.get(id)
        var k = presenter.templates.kindTemplate(kind)
        return k !== "" ? presenter.templates.get(k) : presenter.templates.active
    }
    function openFor(i, fresh) {
        var info = presenter.itemInfo(i)
        itemIndex = i; isNew = fresh; saved = false; kind = info.kind; structureChanged = false
        titleField.text = info.title; subField.text = info.subtitle
        draftTemplate = info.templateId; draftBackground = info.background; draftLoop = info.loopSeconds
        hasRefs = info.refs.length > 0
        slidesModel.clear()
        var s = info.slides, imgs = info.images
        if (s.length === 0 && imgs.length === 0) s = [""]
        var n = Math.max(s.length, imgs.length)
        for (var k = 0; k < n; ++k)
            slidesModel.append({ text: k < s.length ? s[k] : "", image: k < imgs.length ? imgs[k] : "" })
        selectSlide(0)
        open()
    }
    function selectSlide(i) {
        loading = true
        cur = i
        ta.text = slidesModel.get(i).text
        loading = false
        if (!slidesModel.get(i).image) { ta.forceActiveFocus(); ta.cursorPosition = ta.length }
    }
    function save() {
        var arr = [], imgs = []
        for (var k = 0; k < slidesModel.count; ++k) {
            // An empty slide is a real slide: it shows only the background. Delete it with the ✕ button if you do not want it.
            arr.push(slidesModel.get(k).text); imgs.push(slidesModel.get(k).image || "")
        }
        var keep = hasRefs && !structureChanged && arr.length === slidesModel.count
        var title = titleField.text.trim() !== "" ? titleField.text.trim() : "Onderdeel"
        presenter.editItemSlides(itemIndex, title, subField.text.trim(), arr, imgs, draftLoop, keep)
        presenter.setItemTemplate(itemIndex, draftTemplate)
        presenter.setItemBackground(itemIndex, draftBackground)
        saved = true
        close()
    }

    modal: true
    // A popup is positioned relative to the item it is declared in. Move it to the window's content item
    // the first time it opens, so it is centred in the whole window (Window.window is not available on a Dialog itself).
    property bool reparented: false
    onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
    width: parent ? Math.min(1240, parent.width - 40) : 1100
    height: parent ? Math.min(780, parent.height - 40) : 700
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) / 2) : 0
    padding: 16
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
    onClosed: if (isNew && !saved) presenter.removeItem(itemIndex)
    onOpened: { ta.forceActiveFocus(); ta.cursorPosition = ta.length }

    ListModel { id: slidesModel }

    FileDialog {
        id: addFilesDialog
        title: "Kies een presentatie, PDF of afbeeldingen"
        fileMode: FileDialog.OpenFiles
        nameFilters: [presenter.importer.supportedFilter(), "Alle bestanden (*)"]
        onAccepted: presenter.importer.importFiles("editor", selectedFiles)
    }
    Connections {
        target: presenter.importer
        function onFinished(tag, images) {
            if (tag !== "editor" || !dlg.opened) return
            var at = dlg.cur + 1
            for (var k = 0; k < images.length; ++k) slidesModel.insert(at + k, { text: "", image: images[k] })
            dlg.structureChanged = true
            dlg.selectSlide(at)
        }
    }

    contentItem: ColumnLayout {
        spacing: 12

        // ---- header -------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            Text { text: "Scherm bewerken"; color: T.text; font.pixelSize: 20; font.bold: true }
            Item { Layout.fillWidth: true }
            TextField { id: titleField; Layout.preferredWidth: 240; placeholderText: "Titel (in de dienstlijst)"; color: T.text; selectByMouse: true; font.pixelSize: 13
                background: Rectangle { color: T.card; radius: 6; border.color: titleField.activeFocus ? T.accent : T.border } }
            TextField { id: subField; Layout.preferredWidth: 200; placeholderText: "Ondertitel"; color: T.text; selectByMouse: true; font.pixelSize: 13
                background: Rectangle { color: T.card; radius: 6; border.color: subField.activeFocus ? T.accent : T.border } }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 14

            // ---- slides --------------------------------------------------------------------
            ColumnLayout {
                Layout.preferredWidth: 200; Layout.maximumWidth: 200; Layout.fillHeight: true; spacing: 8
                Text { text: "Slides (" + slidesModel.count + ")"; color: T.muted; font.pixelSize: 12 }
                ListView {
                    id: strip
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; spacing: 6; model: slidesModel
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: strip.width - 10; height: width * 9 / 16 + 6; radius: 6; color: "transparent"
                        border.color: index === dlg.cur ? T.accent : T.border; border.width: index === dlg.cur ? 2 : 1
                        SlideView { anchors.fill: parent; anchors.margins: 3; tpl: dlg.tpl; background: dlg.bg; text: model.text; image: model.image || "" }
                        Text { x: 8; y: 4; text: index + 1; color: "white"; font.pixelSize: 11; font.bold: true; style: Text.Outline; styleColor: "#aa000000" }
                        TapHandler { onTapped: dlg.selectSlide(index) }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 4
                    AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 32; leftPadding: 4; rightPadding: 4; text: "+ Slide"
                        onClicked: { dlg.structureChanged = true; slidesModel.insert(dlg.cur + 1, { text: "", image: "" }); dlg.selectSlide(dlg.cur + 1) } }
                    AppButton { implicitHeight: 32; implicitWidth: 34; leftPadding: 0; rightPadding: 0; text: "▲"; enabled: dlg.cur > 0
                        onClicked: { dlg.structureChanged = true; slidesModel.move(dlg.cur, dlg.cur - 1, 1); dlg.cur = dlg.cur - 1 } }
                    AppButton { implicitHeight: 32; implicitWidth: 34; leftPadding: 0; rightPadding: 0; text: "▼"; enabled: dlg.cur < slidesModel.count - 1
                        onClicked: { dlg.structureChanged = true; slidesModel.move(dlg.cur, dlg.cur + 1, 1); dlg.cur = dlg.cur + 1 } }
                    AppButton { implicitHeight: 32; implicitWidth: 34; leftPadding: 0; rightPadding: 0; text: "✕"; enabled: slidesModel.count > 1
                        onClicked: { dlg.structureChanged = true; slidesModel.remove(dlg.cur); dlg.selectSlide(Math.min(dlg.cur, slidesModel.count - 1)) } }
                }
                AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 32; leftPadding: 4; rightPadding: 4
                    text: "+ PowerPoint / PDF / afbeeldingen…"; onClicked: addFilesDialog.open() }
            }

            // ---- the editable slide ---------------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true; Layout.fillHeight: true; spacing: 6
                Item {
                    id: holder
                    Layout.fillWidth: true; Layout.fillHeight: true
                    SlideView {
                        id: canvas
                        anchors.centerIn: parent
                        width: Math.min(holder.width, holder.height * 16 / 9); height: width * 9 / 16
                        tpl: dlg.tpl; background: dlg.bg; text: ""; image: dlg.curImage
                        reference: ""

                        // invisible twin of the slide text: tells us the font size the slide will really use
                        // (long text is shrunk to fit, exactly like on the output)
                        Text {
                            id: measure
                            parent: canvas.editLayer
                            x: ta.x; y: ta.y; width: ta.width; height: ta.height
                            opacity: 0
                            text: ta.text !== "" ? ta.text : "Tekst"
                            font.family: ta.font.family; font.bold: dlg.tpl.bold; font.italic: dlg.tpl.italic
                            font.capitalization: dlg.tpl.uppercase ? Font.AllUppercase : Font.MixedCase
                            font.pixelSize: dlg.tpl.fontSize; lineHeight: dlg.tpl.lineHeight
                            fontSizeMode: Text.Fit; minimumPixelSize: Math.min(40, dlg.tpl.fontSize)
                            wrapMode: Text.WordWrap
                            readonly property int fitted: fontInfo.pixelSize > 0 ? fontInfo.pixelSize : dlg.tpl.fontSize
                        }
                        TextArea {
                            id: ta
                            parent: canvas.editLayer
                            visible: dlg.curImage === ""
                            x: dlg.tpl.margin; y: dlg.tpl.margin
                            width: 1920 - 2 * dlg.tpl.margin
                            height: 1080 - dlg.tpl.margin - (dlg.hasRefs ? Math.max(dlg.tpl.margin, 250) : dlg.tpl.margin)
                            padding: 0
                            wrapMode: TextEdit.Wrap
                            color: dlg.tpl.textColor
                            selectionColor: T.accent; selectedTextColor: "white"
                            placeholderText: "Typ hier de tekst van dit scherm…"; placeholderTextColor: "#80ffffff"
                            font.family: dlg.tpl.fontFamily !== "" ? dlg.tpl.fontFamily : Qt.application.font.family
                            font.pixelSize: measure.fitted
                            font.bold: dlg.tpl.bold; font.italic: dlg.tpl.italic
                            font.capitalization: dlg.tpl.uppercase ? Font.AllUppercase : Font.MixedCase
                            horizontalAlignment: dlg.tpl.align === "left" ? TextEdit.AlignLeft : dlg.tpl.align === "right" ? TextEdit.AlignRight : TextEdit.AlignHCenter
                            verticalAlignment: dlg.tpl.vAlign === "top" ? TextEdit.AlignTop : dlg.tpl.vAlign === "bottom" ? TextEdit.AlignBottom : TextEdit.AlignVCenter
                            selectByMouse: true
                            background: Rectangle { color: "transparent"; radius: 14; border.width: 4
                                border.color: ta.activeFocus ? "#b3ffffff" : "#40ffffff" }
                            onTextChanged: if (!dlg.loading && dlg.cur < slidesModel.count) slidesModel.setProperty(dlg.cur, "text", text)
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true; color: T.muted; font.pixelSize: 11; wrapMode: Text.WordWrap
                    text: "Typ direct op het scherm. Enter = nieuwe regel op dezelfde slide. Een lege slide toont alleen de achtergrond. De regelafstand kan hier iets afwijken; het voorbeeld rechts toont het exacte resultaat."
                }
            }

            // ---- preview + look ---------------------------------------------------------------
            ColumnLayout {
                Layout.preferredWidth: 280; Layout.maximumWidth: 280; Layout.fillHeight: true; spacing: 8
                Text { text: "Zo ziet het publiek het"; color: T.muted; font.pixelSize: 12 }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 280 * 9 / 16   // the column is 280 px wide
                    color: "transparent"; border.color: T.border; radius: 6
                    SlideView {
                        anchors.fill: parent; anchors.margins: 2
                        tpl: dlg.tpl; background: dlg.bg
                        text: dlg.cur < slidesModel.count ? slidesModel.get(dlg.cur).text : ""
                        image: dlg.curImage
                        reference: ""
                    }
                }

                ColumnLayout {   // automatic loop (announcement loop / presentation that runs by itself)
                    visible: dlg.kind === "loop" || dlg.kind === "slides"
                    Layout.fillWidth: true; Layout.topMargin: 4; spacing: 4
                    Text { text: "Automatisch doorlopen"; color: T.muted; font.pixelSize: 12 }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 8
                        AppButton { text: dlg.draftLoop > 0 ? "Aan" : "Uit"; active: dlg.draftLoop > 0; implicitHeight: 32; implicitWidth: 58
                            onClicked: dlg.draftLoop = dlg.draftLoop > 0 ? 0 : 10 }
                        SpinBox { from: 2; to: 300; value: dlg.draftLoop > 0 ? dlg.draftLoop : 10; enabled: dlg.draftLoop > 0
                            implicitWidth: 120; font.pixelSize: 13
                            textFromValue: function (v) { return v + " sec" }
                            onValueModified: dlg.draftLoop = value }
                    }
                    Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 11
                        text: dlg.draftLoop > 0 ? "De slides wisselen vanzelf en beginnen opnieuw. \"Volgende slide\" gaat naar het volgende onderdeel." : "Slides wisselen alleen als jij op Volgende klikt." }
                }

                Text { Layout.topMargin: 6; text: "Template"; color: T.muted; font.pixelSize: 12 }
                ComboBox {
                    id: tplBox
                    Layout.fillWidth: true; font.pixelSize: 13
                    textRole: "name"; valueRole: "id"
                    model: [{ id: "", name: "Standaard (" + dlg.defaultName + ")" }].concat(presenter.templates.all)
                    currentIndex: Math.max(0, indexOfValue(dlg.draftTemplate))
                    onActivated: dlg.draftTemplate = currentValue
                }

                Text { Layout.topMargin: 6; text: "Achtergrond"; color: T.muted; font.pixelSize: 12 }
                GridView {
                    id: bgGrid
                    Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 90
                    clip: true; cellWidth: Math.floor(width / 3); cellHeight: Math.round(cellWidth * 9 / 16) + 6
                    model: [""].concat(["g:sunset", "g:ocean", "g:night", "g:forest", "g:dawn", "g:graphite"], presenter.mediaFiles)
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        width: bgGrid.cellWidth; height: bgGrid.cellHeight
                        Rectangle {
                            anchors.fill: parent; anchors.margins: 3; radius: 5; clip: true; color: T.card
                            border.width: dlg.draftBackground === modelData ? 2 : 1
                            border.color: dlg.draftBackground === modelData ? T.accent : T.border
                            SlideView { anchors.fill: parent; anchors.margins: 2; visible: modelData !== ""; background: modelData; text: ""; tpl: dlg.tpl }
                            Text { visible: modelData === ""; anchors.centerIn: parent; text: "Standaard"; color: T.muted; font.pixelSize: 10 }
                            TapHandler { onTapped: dlg.draftBackground = modelData }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }
            AppButton { text: "Annuleren"; onClicked: dlg.close() }
            AppButton { text: "Opslaan"; primary: true; onClicked: dlg.save() }
        }
    }
}
