import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// Right column: every slide of the current part (click one to put it live) and the live controls.
Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    signal bibleRequested()
    signal overlaysRequested()

    ColumnLayout {
        id: col
        anchors.fill: parent; anchors.margins: 14; spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Text { Layout.fillWidth: true; text: presenter.currentTitle !== "" ? presenter.currentTitle : "Geen onderdeel"
                color: T.text; font.pixelSize: 14; font.bold: true; elide: Text.ElideRight }
            Text { text: (presenter.currentSlide + 1) + "/" + presenter.slideCount; color: T.muted; font.pixelSize: 13 }
        }

        ListView {
            id: slides
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
                        Text { Layout.fillWidth: true; text: modelData; color: T.text; font.pixelSize: 13
                            wrapMode: Text.WordWrap; maximumLineCount: 8; elide: Text.ElideRight }
                        Text { visible: text !== ""; Layout.fillWidth: true; color: T.muted; font.pixelSize: 11
                            text: presenter.currentRefs[index] !== undefined ? presenter.currentRefs[index] : "" }
                    }
                    Text { visible: row.next; text: "volgende"; color: T.muted; font.pixelSize: 11; Layout.alignment: Qt.AlignTop }
                }
            }
        }

        Text {   // what comes after the last slide of this part
            Layout.fillWidth: true; elide: Text.ElideRight; color: T.muted; font.pixelSize: 12
            visible: presenter.currentSlide + 1 >= presenter.slideCount && presenter.nextText !== ""
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
