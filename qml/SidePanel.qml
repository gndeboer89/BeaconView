import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import "Theme.js" as T
import "Backgrounds.js" as Bg

// Bottom-centre library panel: backgrounds (Media), songs (Liederen) and Bible.
// Wide and low, so every tab is laid out horizontally.
Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    clip: true
    property int tab: 0   // 0 = media, 1 = liederen, 2 = bijbel, 3 = overlays
    property int songEdit: -1
    property var bibleResult: ({ ok: false, bookFound: false, error: "", verses: [] })
    property var bibleHits: []

    function focusBible() { tab = 2; refField.forceActiveFocus(); refField.selectAll() }
    function setQuery(q) { refField.text = q }
    function refreshBible() {
        var q = refField.text.trim()
        bibleResult = presenter.bible.lookup(q)
        bibleHits = (!bibleResult.ok && !bibleResult.bookFound) ? presenter.bible.search(q, 40) : []
    }
    function addPassage(show) {
        if (presenter.addPassage(refField.text.trim(), versesPerSlide.value, showNumbers.checked, show) >= 0 && !show)
            refField.selectAll()
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 12; spacing: 10

        RowLayout {
            Layout.fillWidth: true; spacing: 6
            Repeater {
                model: ["Media", "Liederen", "Bijbel", "Overlays"]
                AppButton {
                    Layout.preferredWidth: index === 3 ? 92 : 84; leftPadding: 8; rightPadding: 8; implicitHeight: 34; text: modelData; active: root.tab === index
                    onClicked: index === 2 ? root.focusBible() : root.tab = index
                }
            }
            Item { Layout.fillWidth: true }
            // Context actions of the active tab
            Switch {
                id: applyAll; visible: root.tab === 0; text: "Alle onderdelen"
                contentItem: Text { text: applyAll.text; color: T.muted; font.pixelSize: 13; leftPadding: applyAll.indicator.width + 8; verticalAlignment: Text.AlignVCenter }
            }
            AppButton { visible: root.tab === 0; implicitHeight: 34; text: "Importeren…"; onClicked: fileDialog.open() }
            AppButton { visible: root.tab === 1; implicitHeight: 34; text: "Importeren…"; onClicked: songImport.open() }
            AppButton { visible: root.tab === 1; implicitHeight: 34; text: "Nieuw lied…"
                onClicked: { root.songEdit = -1; songEditor.heading = "Nieuw lied"; songEditor.openWith("", "", "") } }
        }

        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: root.tab

            // ---- Media: backgrounds -------------------------------------------------
            GridView {
                id: grid
                clip: true; cellWidth: 156; cellHeight: (cellWidth - 12) * 9 / 16 + 12
                model: Bg.gradients.map(function (g) { return "g:" + g.id }).concat(presenter.mediaFiles)
                ScrollBar.vertical: ScrollBar {}
                delegate: Item {
                    width: grid.cellWidth; height: grid.cellHeight
                    Rectangle {
                        anchors.fill: parent; anchors.margins: 4; radius: 6; clip: true
                        color: "transparent"
                        border.color: presenter.liveBackground === modelData ? T.accent : T.border
                        border.width: presenter.liveBackground === modelData ? 2 : 1
                        SlideView { anchors.fill: parent; anchors.margins: 2; background: modelData; text: "" }
                        TapHandler { onTapped: {
                            if (applyAll.checked) {
                                presenter.defaultBackground = modelData
                                for (var i = 0; i < presenter.service.rowCount(); ++i) presenter.setItemBackground(i, "")
                            } else {
                                presenter.setItemBackground(presenter.currentItem, modelData)
                            }
                        } }
                    }
                }
            }

            // ---- Liederen -------------------------------------------------------------
            ListView {
                id: songList
                clip: true; spacing: 4
                model: presenter.songs
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    width: songList.width - 8; height: 46; radius: 8; color: T.card
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 6; spacing: 6
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 0
                            Text { Layout.fillWidth: true; text: (number !== "" ? number + "  " : "") + title
                                color: T.text; font.pixelSize: 14; elide: Text.ElideRight }
                            Text {   // credits of an imported song
                                visible: ccli !== "" || author !== ""
                                Layout.fillWidth: true; color: T.muted; font.pixelSize: 11; elide: Text.ElideRight
                                text: (ccli !== "" ? "CCLI " + ccli : "") + (ccli !== "" && author !== "" ? "  ·  " : "") + author
                            }
                        }
                        AppButton { text: "+  Aan dienst"; implicitHeight: 34; onClicked: presenter.addSong(index) }
                        AppButton { text: "✎"; flat: true; implicitWidth: 34; implicitHeight: 34; leftPadding: 0; rightPadding: 0
                            onClicked: { root.songEdit = index; songEditor.heading = "Lied bewerken"
                                songEditor.openWith(title, number, lyrics) } }
                    }
                }
            }

            // ---- Bijbel -----------------------------------------------------------------
            RowLayout {
                spacing: 14

                ColumnLayout {   // input
                    Layout.preferredWidth: 290; Layout.maximumWidth: 290; Layout.fillHeight: true
                    spacing: 6
                    TextField {
                        id: refField
                        Layout.fillWidth: true
                        placeholderText: "Joh 3:16, Psalm 23:1-3 of een woord"
                        color: T.text; selectByMouse: true; font.pixelSize: 14
                        background: Rectangle { color: T.card; radius: 6; border.color: refField.activeFocus ? T.accent : T.border }
                        onTextChanged: root.refreshBible()
                        onAccepted: if (root.bibleResult.ok) root.addPassage(true)
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6
                        ComboBox {
                            id: bookBox
                            Layout.fillWidth: true; Layout.minimumWidth: 0; model: presenter.bible.bookNames; font.pixelSize: 13
                            onActivated: refField.text = presenter.bible.bookName(currentIndex) + " 1"
                        }
                        ComboBox {
                            id: chapterBox
                            Layout.preferredWidth: 70; font.pixelSize: 13
                            model: presenter.bible.chapterCount(bookBox.currentIndex)
                            displayText: currentIndex >= 0 ? String(currentIndex + 1) : ""
                            delegate: ItemDelegate { width: chapterBox.width; text: index + 1; font.pixelSize: 13; highlighted: chapterBox.highlightedIndex === index }
                            onActivated: refField.text = presenter.bible.bookName(bookBox.currentIndex) + " " + (currentIndex + 1)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6
                        Text { text: "Verzen/slide"; color: T.muted; font.pixelSize: 12 }
                        SpinBox { id: versesPerSlide; from: 1; to: 8; value: 2; font.pixelSize: 13; editable: false; implicitWidth: 110 }
                        CheckBox { id: showNumbers; checked: true; text: "Nrs."
                            contentItem: Text { text: showNumbers.text; color: T.muted; font.pixelSize: 12; leftPadding: showNumbers.indicator.width + 4; verticalAlignment: Text.AlignVCenter } }
                    }
                    Item { Layout.fillHeight: true }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 6
                        AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 36; leftPadding: 8; rightPadding: 8; text: "Toevoegen"; enabled: root.bibleResult.ok; onClicked: root.addPassage(false) }
                        AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 36; leftPadding: 8; rightPadding: 8; text: "Toevoegen + tonen"; primary: true; enabled: root.bibleResult.ok; onClicked: root.addPassage(true) }
                    }
                }

                ColumnLayout {   // verses / search results
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 4
                    Text {
                        Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 13
                        color: root.bibleResult.ok ? T.ok : (root.bibleResult.bookFound ? T.danger : T.muted)
                        text: refField.text.trim() === "" ? "Bijbel (" + presenter.bible.translation + ") – typ een tekst of zoek een woord"
                            : root.bibleResult.ok ? root.bibleResult.label
                            : root.bibleResult.bookFound ? root.bibleResult.error
                            : (root.bibleHits.length > 0 ? root.bibleHits.length + (root.bibleHits.length >= 40 ? "+" : "") + " resultaten" : "Geen resultaten")
                    }
                    ListView {
                        id: verseList
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true; spacing: 2
                        model: root.bibleResult.ok ? root.bibleResult.verses : root.bibleHits
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Rectangle {
                            width: verseList.width - 8
                            height: vcol.implicitHeight + 10; radius: 6
                            color: root.bibleResult.ok ? "transparent" : (hit.hovered ? T.card : "transparent")
                            HoverHandler { id: hit }
                            TapHandler { enabled: !root.bibleResult.ok; onTapped: if (modelData.ref !== undefined) refField.text = modelData.ref }
                            ColumnLayout {
                                id: vcol
                                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                                anchors.margins: 5; spacing: 2
                                Text { visible: !root.bibleResult.ok; text: modelData.ref !== undefined ? modelData.ref : ""; color: T.accent; font.pixelSize: 12; font.bold: true }
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 8
                                    Text { visible: root.bibleResult.ok; text: modelData.n !== undefined ? modelData.n : ""; color: T.accent; font.pixelSize: 13; font.bold: true
                                        Layout.alignment: Qt.AlignTop; Layout.preferredWidth: 24 }
                                    Text { Layout.fillWidth: true; text: modelData.text; color: T.text; font.pixelSize: 13; wrapMode: Text.WordWrap }
                                }
                            }
                        }
                    }
                }
            }

            // ---- Overlays ---------------------------------------------------------------
            OverlayPanel {}
        }
    }

    FileDialog {
        id: fileDialog
        title: "Kies een afbeelding of video"
        nameFilters: ["Afbeeldingen en video's (*.png *.jpg *.jpeg *.webp *.bmp *.mp4 *.m4v *.mov *.webm *.mkv)", "Alle bestanden (*)"]
        onAccepted: presenter.importMedia(selectedFile)
    }
    FileDialog {
        id: songImport
        title: "Kies liedteksten uit SongSelect (.txt of .usr)"
        fileMode: FileDialog.OpenFiles
        nameFilters: ["Liedteksten (*.txt *.usr)", "Alle bestanden (*)"]
        onAccepted: {
            var r = presenter.songs.importFiles(selectedFiles)
            var parts = []
            if (r.added > 0) parts.push(r.added + (r.added === 1 ? " lied toegevoegd" : " liederen toegevoegd"))
            if (r.existing > 0) parts.push(r.existing + (r.existing === 1 ? " stond er al in" : " stonden er al in"))
            if (r.failed > 0) parts.push(r.failed + (r.failed === 1 ? " bestand kon niet worden gelezen" : " bestanden konden niet worden gelezen"))
            importResult.text = parts.join(";  ")
            importResult.detail = r.failed > 0 ? "Gebruik het tekstbestand (.txt) of het USR-bestand (.usr) van SongSelect." : (r.titles.length > 0 ? r.titles.join("\n") : "")
            importResult.open()
        }
    }
    Dialog {
        id: importResult
        property string text: ""
        property string detail: ""
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Liederen importeren"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.text; font.pixelSize: 14; text: importResult.text }
            Text { visible: importResult.detail !== ""; Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 13; text: importResult.detail }
            AppButton { Layout.alignment: Qt.AlignRight; text: "OK"; primary: true; onClicked: importResult.close() }
        }
    }
    TextEditorDialog {
        id: songEditor
        subtitleLabel: "Liednummer (optioneel)"
        bodyLabel: "Tekst: een lege regel = nieuwe slide. Een regel als [Couplet 1] boven een deel is de naam van dat deel (komt niet op het scherm)"
        onConfirmed: function (t, s, b) {
            if (root.songEdit >= 0) presenter.songs.update(root.songEdit, t, s, b)
            else presenter.songs.add(t, s, b)
        }
    }
}
