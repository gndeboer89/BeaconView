import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Window
import "Theme.js" as T

// Songs: the list on the left, the editor on the right (title, bundle, tags, order of the parts, lyrics, preview).
Item {
    id: root
    property int row: -2                  // -2 nothing chosen, -1 a new song, 0.. a row of the library
    property double songId: 0             // the id of the open song in the database (the row can move when the other program changes songs)
    property string notice: ""            // a message about what the other program did
    property var orig: ({})               // what was loaded; the page is "changed" when the fields differ from it
    property int pendingRow: -3           // the row to open after the question about unsaved changes

    readonly property var current: ({
        title: titleField.text.trim(), number: numberField.text.trim(), lyrics: lyricsArea.text, order: orderField.text.trim(),
        category: categoryField.text.trim(), tags: tagList(), author: authorField.text.trim(), ccli: ccliField.text.replace(/\D/g, "")
    })
    readonly property bool changed: row !== -2 && (row === -1
        ? (titleField.text.trim() !== "" || lyricsArea.text.trim() !== "")
        : (current.title !== orig.title || current.number !== orig.number || current.lyrics !== orig.lyrics || current.order !== orig.order
           || current.category !== orig.category || current.tags.join("|") !== orig.tags.join("|")
           || current.author !== orig.author || current.ccli !== orig.ccli))

    function tagList() {
        return tagsField.text.split(/[,;\n]/).map(function (t) { return t.trim().replace(/^#+/, "") }).filter(function (t) { return t !== "" })
    }
    function load(r) {
        var m = r >= 0 ? presenter.songs.get(r) : { title: "", number: "", lyrics: "", order: "", category: "", tags: [], author: "", ccli: "" }
        orig = { title: m.title, number: m.number, lyrics: m.lyrics, order: m.order, category: m.category, tags: m.tags, author: m.author, ccli: m.ccli,
                 rev: m.rev !== undefined ? m.rev : 0 }
        songId = m.id !== undefined ? m.id : 0
        notice = ""

        titleField.text = m.title; numberField.text = m.number; lyricsArea.text = m.lyrics; orderField.text = m.order
        categoryField.text = m.category; tagsField.text = m.tags.join(", "); authorField.text = m.author; ccliField.text = m.ccli
        row = r                                          // last: the fields and `orig` are ready when the page compares them
        if (r === -1) titleField.forceActiveFocus()
    }
    // Opens a song (or -1: a new one), after asking what to do with unsaved changes.
    function openSong(r) {
        if (changed && r !== row) { pendingRow = r; leaveDialog.open() } else load(r)
    }
    function save(force) {
        if (row === -2) return false                       // nothing is open
        var c = current
        if (c.title === "") return false
        if (row === -1) {
            var id = presenter.songs.add(c.title, c.number, c.lyrics, c.order, c.category, c.tags)
            if (id === 0) return false
            row = presenter.songs.rowOfId(id)
        } else {
            // the revision says which version this page started from: when the other program changed the song since, nothing is overwritten
            if (!presenter.songs.update(row, c.title, c.number, c.lyrics, c.order, c.category, c.tags, force === true ? -1 : orig.rev)) {
                if (presenter.songs.rowOfId(songId) >= 0) conflictDialog.open()
                return false
            }
        }
        presenter.songs.setCredits(row, c.author, c.ccli)
        load(row)
        return true
    }
    // The other program changed songs: find this song again, and tell what happened to it.
    function onExternal() {
        if (songId === 0) return                           // a new song, or nothing open
        var r = presenter.songs.rowOfId(songId)
        if (r < 0) {                                       // removed in the other program: what is on this page can still be saved as a new song
            row = -1; songId = 0
            notice = "Dit lied is in het andere programma verwijderd. Je kunt het opslaan als nieuw lied."
            return
        }
        row = r
        var m = presenter.songs.get(r)
        if (m.rev === orig.rev) return                     // not this song
        if (!changed) { load(r); notice = "Dit lied is bijgewerkt in het andere programma."; return }
        notice = "Dit lied is ondertussen ook in het andere programma gewijzigd. Bij opslaan kun je kiezen welke versie blijft."
    }
    function insertPart(name) {   // adds "[Couplet 2]" at the end of the lyrics
        var n = name, t = lyricsArea.text
        if (name === "Couplet") {
            var count = (t.match(/^\[Couplet \d+\]/gm) || []).length
            n = "Couplet " + (count + 1)
        }
        lyricsArea.text = (t.trim() === "" ? "" : t.replace(/\s+$/, "") + "\n\n") + "[" + n + "]\n"
        lyricsArea.forceActiveFocus(); lyricsArea.cursorPosition = lyricsArea.length
    }
    Connections { target: presenter.songs; function onExternallyChanged() { root.onExternal() } }
    readonly property var parts: presenter.partNames(lyricsArea.text)
    readonly property var preview: presenter.songPreview(lyricsArea.text, orderField.text)

    RowLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 14

        // ---- list ------------------------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 380; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 12; spacing: 8
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: "Liederen"; color: T.text; font.pixelSize: 20; font.bold: true; Layout.fillWidth: true; elide: Text.ElideRight }
                    AppButton { visible: presenter.hasService; text: "↗"; implicitHeight: 32; implicitWidth: 36; leftPadding: 0; rightPadding: 0
                        ToolTip.visible: hovered; ToolTip.text: "Liedprogramma openen: een apart programma voor de liederen, dat naast BeaconView kan openstaan"
                        onClicked: if (!presenter.openSongStudio()) root.notice = "Het liedprogramma is niet gevonden. Installeer BeaconView opnieuw." }
                    AppButton { text: "Importeren…"; implicitHeight: 32; leftPadding: 10; rightPadding: 10; onClicked: songImport.open() }
                    AppButton { text: "+ Nieuw"; primary: true; implicitHeight: 32; leftPadding: 10; rightPadding: 10; onClicked: root.openSong(-1) }
                }
                TextField {
                    id: search
                    Layout.fillWidth: true; implicitHeight: 34; placeholderText: "Zoeken in titel, tekst, auteur, bundel of tag"
                    color: T.text; selectByMouse: true; font.pixelSize: 13
                    background: Rectangle { color: T.card; radius: 6; border.color: search.activeFocus ? T.accent : T.border }
                    onTextChanged: presenter.songFilter.query = text
                    Keys.onEscapePressed: text = ""
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    ComboBox {
                        Layout.fillWidth: true; implicitHeight: 32; font.pixelSize: 13
                        visible: presenter.songs.categories.length > 0
                        model: ["Alle bundels"].concat(presenter.songs.categories)
                        currentIndex: Math.max(0, model.indexOf(presenter.songFilter.category))
                        onActivated: presenter.songFilter.category = currentIndex === 0 ? "" : model[currentIndex]
                    }
                    ComboBox {
                        Layout.fillWidth: true; implicitHeight: 32; font.pixelSize: 13
                        visible: presenter.songs.tags.length > 0
                        model: ["Alle tags"].concat(presenter.songs.tags)
                        currentIndex: Math.max(0, model.indexOf(presenter.songFilter.tag))
                        onActivated: presenter.songFilter.tag = currentIndex === 0 ? "" : model[currentIndex]
                    }
                }
                Text { color: T.muted; font.pixelSize: 12
                    text: list.count === presenter.songFilter.total ? presenter.songFilter.total + (presenter.songFilter.total === 1 ? " lied" : " liederen")
                                                                    : list.count + " van " + presenter.songFilter.total }
                ListView {
                    id: list
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 3
                    model: presenter.songFilter
                    ScrollBar.vertical: ScrollBar {}
                    Text {
                        visible: list.count === 0; anchors.centerIn: parent; width: parent.width - 30; horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 13
                        text: presenter.songFilter.total === 0 ? "Nog geen liederen. Maak er een met 'Nieuw lied' of importeer ze uit SongSelect."
                                                               : "Geen lied gevonden met deze zoekopdracht en filters."
                    }
                    delegate: Rectangle {
                        id: item
                        required property int index
                        required property string title
                        required property string number
                        required property string category
                        required property var tags
                        readonly property int src: presenter.songFilter.sourceRow(index)
                        readonly property bool chosen: src === root.row
                        width: list.width - 8; height: 48; radius: 8
                        color: chosen ? T.accentSoft : (hover.hovered ? T.cardHover : T.card)
                        border.color: chosen ? T.accent : "transparent"
                        HoverHandler { id: hover; cursorShape: Qt.PointingHandCursor }
                        TapHandler { onTapped: root.openSong(item.src) }
                        ColumnLayout {
                            anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 0
                            Item { Layout.fillHeight: true }
                            Text { Layout.fillWidth: true; text: (item.number !== "" ? item.number + "  " : "") + item.title; color: T.text; font.pixelSize: 14; elide: Text.ElideRight }
                            Text {
                                readonly property string line: [item.category, item.tags.map(function (t) { return "#" + t }).join(" ")].filter(function (x) { return x !== "" }).join("  ·  ")
                                visible: line !== ""; Layout.fillWidth: true; text: line; color: T.muted; font.pixelSize: 11; elide: Text.ElideRight
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }

        // ---- editor ----------------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: T.panel; radius: 12; border.color: T.border

            Text {   // nothing chosen
                visible: root.row === -2; anchors.centerIn: parent; color: T.muted; font.pixelSize: 15
                text: "Kies links een lied om te bewerken, of maak een nieuw lied."
            }

            ColumnLayout {
                visible: root.row !== -2
                anchors.fill: parent; anchors.margins: 16; spacing: 10

                Rectangle {   // the songs cannot be saved
                    visible: presenter.songs.storageError !== ""
                    Layout.fillWidth: true; implicitHeight: warn.implicitHeight + 16; radius: 8; color: "#3b1717"; border.color: "#b3261e"
                    Text { id: warn; anchors.fill: parent; anchors.margins: 8; wrapMode: Text.WordWrap; color: "#ffb4ab"; font.pixelSize: 13
                        text: "De liederen worden niet opgeslagen: " + presenter.songs.storageError }
                }
                Rectangle {   // what the other program did
                    visible: root.notice !== ""
                    Layout.fillWidth: true; implicitHeight: note.implicitHeight + 16; radius: 8; color: T.accentSoft; border.color: T.accent
                    Text { id: note; anchors.fill: parent; anchors.margins: 8; wrapMode: Text.WordWrap; color: T.text; font.pixelSize: 13; text: root.notice }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: root.row === -1 ? "Nieuw lied" : "Lied bewerken"; color: T.text; font.pixelSize: 20; font.bold: true }
                    Text { visible: root.changed; text: "● niet opgeslagen"; color: "#ffb454"; font.pixelSize: 12 }
                    Item { Layout.fillWidth: true }
                    AppButton { text: "Herstellen"; implicitHeight: 34; enabled: root.changed; onClicked: root.load(root.row) }
                    AppButton { text: "Opslaan"; primary: true; implicitHeight: 34; enabled: root.changed && titleField.text.trim() !== ""; onClicked: root.save() }
                }

                // title, number, author, CCLI
                GridLayout {
                    Layout.fillWidth: true; columns: 4; columnSpacing: 10; rowSpacing: 4
                    Text { text: "Titel"; color: T.muted; font.pixelSize: 12; Layout.columnSpan: 2 }
                    Text { text: "Liednummer"; color: T.muted; font.pixelSize: 12 }
                    Text { text: "CCLI-nummer"; color: T.muted; font.pixelSize: 12 }
                    TextField { id: titleField; Layout.fillWidth: true; Layout.columnSpan: 2; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                        background: Rectangle { color: T.card; radius: 6; border.color: titleField.activeFocus ? T.accent : T.border } }
                    TextField { id: numberField; Layout.preferredWidth: 120; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                        background: Rectangle { color: T.card; radius: 6; border.color: numberField.activeFocus ? T.accent : T.border } }
                    TextField { id: ccliField; Layout.preferredWidth: 140; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                        placeholderText: "(voor het rapport)"
                        background: Rectangle { color: T.card; radius: 6; border.color: ccliField.activeFocus ? T.accent : T.border } }
                    Text { text: "Auteur"; color: T.muted; font.pixelSize: 12; Layout.columnSpan: 2 }
                    Text { text: "Bundel"; color: T.muted; font.pixelSize: 12; Layout.columnSpan: 2 }
                    TextField { id: authorField; Layout.fillWidth: true; Layout.columnSpan: 2; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                        background: Rectangle { color: T.card; radius: 6; border.color: authorField.activeFocus ? T.accent : T.border } }
                    TextField { id: categoryField; Layout.fillWidth: true; Layout.columnSpan: 2; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                        placeholderText: "bijvoorbeeld Opwekking, Hillsong, Liedboek"
                        background: Rectangle { color: T.card; radius: 6; border.color: categoryField.activeFocus ? T.accent : T.border } }
                }
                Flow {   // bundles that exist
                    Layout.fillWidth: true; spacing: 6; visible: presenter.songs.categories.length > 0
                    Text { text: "Bundel:"; color: T.muted; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                    Repeater {
                        model: presenter.songs.categories.slice(0, 14)
                        AppButton { required property string modelData; text: modelData; implicitHeight: 28; leftPadding: 10; rightPadding: 10
                            active: categoryField.text.trim().toLowerCase() === modelData.toLowerCase(); onClicked: categoryField.text = modelData }
                    }
                }
                Text { text: "Tags, gescheiden door komma's"; color: T.muted; font.pixelSize: 12 }
                TextField { id: tagsField; Layout.fillWidth: true; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                    placeholderText: "bijvoorbeeld Kerst, Pasen"
                    background: Rectangle { color: T.card; radius: 6; border.color: tagsField.activeFocus ? T.accent : T.border } }
                Flow {
                    Layout.fillWidth: true; spacing: 6; visible: presenter.songs.tags.length > 0
                    Text { text: "Tags:"; color: T.muted; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                    Repeater {
                        model: presenter.songs.tags.slice(0, 20)
                        AppButton {
                            required property string modelData
                            readonly property bool on: root.tagList().some(function (t) { return t.toLowerCase() === modelData.toLowerCase() })
                            text: (on ? "✓ " : "+ ") + modelData; implicitHeight: 28; leftPadding: 10; rightPadding: 10; active: on
                            onClicked: {
                                var key = modelData.toLowerCase(), cur = root.tagList()
                                tagsField.text = (on ? cur.filter(function (t) { return t.toLowerCase() !== key }) : cur.concat([modelData])).join(", ")
                            }
                        }
                    }
                }

                // lyrics and preview
                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 6
                        RowLayout {
                            Layout.fillWidth: true; spacing: 6
                            Text { text: "Tekst: elk deel eenmaal, met een regel [Couplet 1] erboven; een lege regel = nieuwe slide"; color: T.muted; font.pixelSize: 12
                                Layout.fillWidth: true; elide: Text.ElideRight }
                            Text { text: "Deel toevoegen:"; color: T.muted; font.pixelSize: 12 }
                            AppButton { text: "Couplet"; implicitHeight: 26; leftPadding: 8; rightPadding: 8; onClicked: root.insertPart("Couplet") }
                            AppButton { text: "Refrein"; implicitHeight: 26; leftPadding: 8; rightPadding: 8; onClicked: root.insertPart("Refrein") }
                            AppButton { text: "Brug"; implicitHeight: 26; leftPadding: 8; rightPadding: 8; onClicked: root.insertPart("Brug") }
                        }
                        ScrollView {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            TextArea { id: lyricsArea; color: T.text; selectByMouse: true; wrapMode: TextEdit.Wrap; font.pixelSize: 14
                                background: Rectangle { color: T.card; radius: 6; border.color: lyricsArea.activeFocus ? T.accent : T.border } }
                        }
                        Text { text: "Volgorde van de delen (leeg = zoals ze er staan)"; color: T.muted; font.pixelSize: 12 }
                        TextField { id: orderField; Layout.fillWidth: true; implicitHeight: 34; color: T.text; selectByMouse: true; font.pixelSize: 14
                            placeholderText: "bijvoorbeeld Couplet 1, Refrein, Couplet 2, Refrein"
                            background: Rectangle { color: T.card; radius: 6; border.color: orderField.activeFocus ? T.accent : T.border } }
                        Flow {
                            Layout.fillWidth: true; spacing: 6; visible: root.parts.length > 0
                            Text { text: "Voeg toe:"; color: T.muted; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                            Repeater {
                                model: root.parts
                                AppButton { required property string modelData; text: "+ " + modelData; implicitHeight: 28; leftPadding: 10; rightPadding: 10
                                    onClicked: orderField.text = (orderField.text.trim() === "" ? "" : orderField.text.trim() + ", ") + modelData }
                            }
                        }
                    }

                    // what the congregation gets: the slides in the chosen order
                    ColumnLayout {
                        Layout.preferredWidth: 300; Layout.fillHeight: true; spacing: 6
                        Text { text: "Slides in deze volgorde (" + root.preview.length + ")"; color: T.muted; font.pixelSize: 12 }
                        ListView {
                            id: previewList
                            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 5
                            model: root.preview
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Rectangle {
                                required property int index
                                required property var modelData
                                width: previewList.width - 10; height: slideText.implicitHeight + 28; radius: 8; color: T.card; border.color: T.border
                                ColumnLayout {
                                    anchors.fill: parent; anchors.margins: 8; spacing: 2
                                    Text { visible: modelData.label !== ""; text: modelData.label; color: T.accent; font.pixelSize: 11; font.bold: true }
                                    Text { id: slideText; Layout.fillWidth: true; text: modelData.text; color: T.text; font.pixelSize: 13; wrapMode: Text.WordWrap; maximumLineCount: 6; elide: Text.ElideRight }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    AppButton { text: "Aan dienst toevoegen"; implicitHeight: 34; visible: presenter.hasService; enabled: root.row >= 0
                        onClicked: { if (root.changed && !root.save()) return; presenter.addSong(root.row); addedNote.visible = true; addedTimer.restart() } }
                    Text { id: addedNote; visible: false; text: "✓ Toegevoegd aan de dienst"; color: T.ok; font.pixelSize: 12
                        Timer { id: addedTimer; interval: 2500; onTriggered: addedNote.visible = false } }
                    Item { Layout.fillWidth: true }
                    AppButton { text: "Verwijderen…"; implicitHeight: 34; enabled: root.row >= 0; onClicked: deleteDialog.open() }
                }
            }
        }
    }

    SongImport { id: songImport }

    // ---- questions ----------------------------------------------------------------
    Dialog {
        id: conflictDialog
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 500; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Dit lied is ondertussen gewijzigd"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: "Het lied is na het openen ook in het andere programma opgeslagen. Welke versie wil je houden?" }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: conflictDialog.close() }
                AppButton { text: "Hun versie laden"; onClicked: { conflictDialog.close(); root.load(presenter.songs.rowOfId(root.songId)) } }
                AppButton { text: "Mijn versie bewaren"; primary: true; onClicked: { conflictDialog.close(); root.save(true) } }
            }
        }
    }
    Dialog {
        id: leaveDialog
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Wijzigingen opslaan?"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: "Dit lied heeft wijzigingen die nog niet zijn opgeslagen." }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: leaveDialog.close() }
                AppButton { text: "Weggooien"; onClicked: { leaveDialog.close(); root.load(root.pendingRow) } }
                AppButton { text: "Opslaan"; primary: true; enabled: titleField.text.trim() !== ""
                    onClicked: { leaveDialog.close(); if (root.save()) root.load(root.pendingRow) } }
            }
        }
    }
    Dialog {
        id: deleteDialog
        modal: true
        property bool reparented: false
        onAboutToShow: if (!reparented && parent && parent.Window.window) { reparented = true; parent = parent.Window.window.contentItem }
        x: parent ? Math.round((parent.width - width) / 2) : 0
        y: parent ? Math.round((parent.height - height) / 2) : 0
        width: 460; padding: 20
        background: Rectangle { color: T.panel; radius: 12; border.color: T.border }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "Lied verwijderen?"; color: T.text; font.pixelSize: 20; font.bold: true }
            Text { Layout.fillWidth: true; wrapMode: Text.WordWrap; color: T.muted; font.pixelSize: 14
                text: "'" + root.orig.title + "' wordt uit de bibliotheek verwijderd. Onderdelen in de dienst waar dit lied al in staat, blijven staan." }
            RowLayout {
                Layout.alignment: Qt.AlignRight; spacing: 8
                AppButton { text: "Annuleren"; onClicked: deleteDialog.close() }
                AppButton { text: "Verwijderen"; primary: true
                    onClicked: { deleteDialog.close(); var r = root.row; root.row = -2; presenter.songs.remove(r) } }
            }
        }
    }
}
