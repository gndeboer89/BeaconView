import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "Theme.js" as T
import "Backgrounds.js" as Bg

Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    clip: true
    property int tab: 0   // 0 = media, 1 = liederen, 2 = bijbel
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
        anchors.fill: parent; anchors.margins: 12; spacing: 12

        RowLayout {
            Layout.fillWidth: true; spacing: 6
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Media"; active: root.tab === 0; onClicked: root.tab = 0 }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Liederen"; active: root.tab === 1; onClicked: root.tab = 1 }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Bijbel"; active: root.tab === 2; onClicked: root.focusBible() }
        }

        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: root.tab

            // ---- Media: backgrounds -------------------------------------------------
            ColumnLayout {
                spacing: 10
                Text { text: "Achtergronden"; color: T.text; font.pixelSize: 15; font.bold: true }
                Switch { id: applyAll; text: "Voor alle onderdelen gebruiken"
                    contentItem: Text { text: applyAll.text; color: T.muted; font.pixelSize: 13; leftPadding: applyAll.indicator.width + 8; verticalAlignment: Text.AlignVCenter } }
                GridView {
                    id: grid
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; cellWidth: width / 2; cellHeight: (cellWidth - 12) * 9 / 16 + 12
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
                AppButton { Layout.fillWidth: true; text: "Afbeelding importeren…"; onClicked: fileDialog.open() }
            }

            // ---- Liederen -------------------------------------------------------------
            ColumnLayout {
                spacing: 10
                Text { text: "Liederen (" + presenter.songs.rowCount() + ")"; color: T.text; font.pixelSize: 15; font.bold: true }
                ListView {
                    id: songList
                    Layout.fillWidth: true; Layout.fillHeight: true
                    clip: true; spacing: 4
                    model: presenter.songs
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: songList.width - 8; height: 52; radius: 8; color: T.card
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 10; anchors.rightMargin: 6; spacing: 6
                            Text { Layout.fillWidth: true; text: (number !== "" ? number + "  " : "") + title
                                color: T.text; font.pixelSize: 14; elide: Text.ElideRight }
                            AppButton { text: "+"; implicitWidth: 34; implicitHeight: 34; leftPadding: 0; rightPadding: 0
                                onClicked: presenter.addSong(index) }
                            AppButton { text: "✎"; flat: true; implicitWidth: 34; implicitHeight: 34; leftPadding: 0; rightPadding: 0
                                onClicked: { root.songEdit = index; songEditor.heading = "Lied bewerken"
                                    songEditor.openWith(title, number, lyrics) } }
                        }
                    }
                }
                AppButton { Layout.fillWidth: true; text: "Nieuw lied…"
                    onClicked: { root.songEdit = -1; songEditor.heading = "Nieuw lied"; songEditor.openWith("", "", "") } }
            }

            // ---- Bijbel -----------------------------------------------------------------
            ColumnLayout {
                spacing: 8
                Text { text: "Bijbel (" + presenter.bible.translation + ")"; color: T.text; font.pixelSize: 15; font.bold: true }
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
                        Layout.fillWidth: true; model: presenter.bible.bookNames; font.pixelSize: 13
                        onActivated: refField.text = presenter.bible.bookName(currentIndex) + " 1"
                    }
                    ComboBox {
                        id: chapterBox
                        Layout.preferredWidth: 78; font.pixelSize: 13
                        model: presenter.bible.chapterCount(bookBox.currentIndex)
                        displayText: currentIndex >= 0 ? String(currentIndex + 1) : ""
                        delegate: ItemDelegate { width: chapterBox.width; text: index + 1; font.pixelSize: 13; highlighted: chapterBox.highlightedIndex === index }
                        onActivated: refField.text = presenter.bible.bookName(bookBox.currentIndex) + " " + (currentIndex + 1)
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 8
                    Text { text: "Verzen/slide"; color: T.muted; font.pixelSize: 13 }
                    SpinBox { id: versesPerSlide; from: 1; to: 8; value: 2; font.pixelSize: 13; editable: false }
                    Item { Layout.fillWidth: true }
                    CheckBox { id: showNumbers; checked: true; text: "Nrs."
                        contentItem: Text { text: showNumbers.text; color: T.muted; font.pixelSize: 13; leftPadding: showNumbers.indicator.width + 6; verticalAlignment: Text.AlignVCenter } }
                }
                Text {
                    Layout.fillWidth: true; wrapMode: Text.WordWrap; font.pixelSize: 13
                    visible: refField.text.trim() !== ""
                    color: root.bibleResult.ok ? T.ok : (root.bibleResult.bookFound ? T.danger : T.muted)
                    text: root.bibleResult.ok ? root.bibleResult.label
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
                        height: col.implicitHeight + 10; radius: 6
                        color: root.bibleResult.ok ? "transparent" : (hit.hovered ? T.card : "transparent")
                        HoverHandler { id: hit }
                        TapHandler { enabled: !root.bibleResult.ok; onTapped: if (modelData.ref !== undefined) refField.text = modelData.ref }
                        ColumnLayout {
                            id: col
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
                RowLayout {
                    Layout.fillWidth: true; spacing: 6
                    AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Toevoegen"; enabled: root.bibleResult.ok; onClicked: root.addPassage(false) }
                    AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Toevoegen + tonen"; primary: true; enabled: root.bibleResult.ok; onClicked: root.addPassage(true) }
                }
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: "Kies een achtergrondafbeelding"
        nameFilters: ["Afbeeldingen (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: presenter.importMedia(selectedFile)
    }
    TextEditorDialog {
        id: songEditor
        subtitleLabel: "Liednummer (optioneel)"
        onConfirmed: function (t, s, b) {
            if (root.songEdit >= 0) presenter.songs.update(root.songEdit, t, s, b)
            else presenter.songs.add(t, s, b)
        }
    }
}
