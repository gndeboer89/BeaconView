import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// Right column: current + next slide and the live controls.
Rectangle {
    id: root
    color: T.panel; radius: 12; border.color: T.border
    signal bibleRequested()
    signal overlaysRequested()

    ColumnLayout {
        id: col
        anchors.fill: parent; anchors.margins: 14; spacing: 8

        Text { text: "Huidige slide (" + (presenter.currentSlide + 1) + "/" + presenter.slideCount + ")"; color: T.muted; font.pixelSize: 13 }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: col.width * 9 / 16
            color: "transparent"; border.color: T.accent; border.width: 2; radius: 6
            SlideView { anchors.fill: parent; anchors.margins: 3; text: presenter.liveText; reference: presenter.liveReference; background: presenter.liveBackground }
        }

        RowLayout {
            Layout.fillWidth: true; spacing: 8
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "‹  Vorige"; onClicked: presenter.prevSlide() }
            AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; text: "Volgende  ›"; onClicked: presenter.nextSlide() }
        }

        Text { Layout.topMargin: 6; text: "Volgende slide"; color: T.muted; font.pixelSize: 13 }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: col.width * 9 / 16
            color: "transparent"; border.color: T.border; radius: 6
            SlideView { anchors.fill: parent; anchors.margins: 3; text: presenter.nextText; reference: presenter.nextReference; background: presenter.nextBackground }
        }

        Item { Layout.fillHeight: true }

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
