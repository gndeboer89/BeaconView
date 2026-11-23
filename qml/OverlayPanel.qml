import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "Theme.js" as T

// "Overlays" tab: timer on the left, message bars + clock on the right.
RowLayout {
    id: root
    spacing: 14
    readonly property var ov: presenter.overlays
    property int editIndex: -1

    // ---- timer ------------------------------------------------------------------------
    Rectangle {
        Layout.preferredWidth: 360; Layout.maximumWidth: 360; Layout.fillHeight: true
        color: T.card; radius: 10; border.color: T.border
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 12; spacing: 8

            RowLayout {
                Layout.fillWidth: true; spacing: 6
                Text { text: "Timer"; color: T.text; font.pixelSize: 15; font.bold: true }
                Item { Layout.fillWidth: true }
                AppButton { text: "Aftellen"; implicitHeight: 30; leftPadding: 10; rightPadding: 10; active: root.ov.timerMode === 0; onClicked: root.ov.timerMode = 0 }
                AppButton { text: "Stopwatch"; implicitHeight: 30; leftPadding: 10; rightPadding: 10; active: root.ov.timerMode === 1; onClicked: root.ov.timerMode = 1 }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 6
                visible: root.ov.timerMode === 0
                SpinBox { id: minBox; from: 0; to: 180; implicitWidth: 114; font.pixelSize: 13
                    value: Math.floor(root.ov.timerDuration / 60)
                    textFromValue: function (v) { return v + " min" }
                    onValueModified: root.ov.timerDuration = value * 60 + secBox.value }
                SpinBox { id: secBox; from: 0; to: 59; implicitWidth: 114; font.pixelSize: 13
                    value: root.ov.timerDuration % 60
                    textFromValue: function (v) { return v + " sec" }
                    onValueModified: root.ov.timerDuration = minBox.value * 60 + value }
                Item { Layout.fillWidth: true }
                Repeater {
                    model: [1, 5, 10]
                    AppButton { text: modelData + "'"; implicitHeight: 30; implicitWidth: 34; leftPadding: 0; rightPadding: 0
                        onClicked: root.ov.timerDuration = modelData * 60 }
                }
            }

            TextField {
                Layout.fillWidth: true; text: root.ov.timerLabel; color: T.text; selectByMouse: true; font.pixelSize: 13
                placeholderText: "Tekst boven de timer (optioneel)"
                background: Rectangle { color: T.bg; radius: 6; border.color: parent.activeFocus ? T.accent : T.border }
                onEditingFinished: root.ov.timerLabel = text
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Text {
                    text: root.ov.timerText; font.pixelSize: 38; font.bold: true
                    color: root.ov.timerFinished ? T.danger : T.text
                    Layout.preferredWidth: 120
                }
                Item { Layout.fillWidth: true }
                AppButton { text: root.ov.timerRunning ? "⏸  Pauze" : "▶  Start"; primary: true; implicitHeight: 38; implicitWidth: 96
                    onClicked: root.ov.toggleRunning() }
                AppButton { text: "↺"; implicitHeight: 38; implicitWidth: 40; leftPadding: 0; rightPadding: 0; onClicked: root.ov.reset() }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 6
                AppButton { Layout.fillWidth: true; Layout.minimumWidth: 0; implicitHeight: 34; text: root.ov.timerShown ? "Op scherm: aan" : "Op scherm: uit"
                    active: root.ov.timerShown; onClicked: root.ov.timerShown = !root.ov.timerShown }
                AppButton { Layout.preferredWidth: 84; Layout.minimumWidth: 0; implicitHeight: 34; text: root.ov.timerCorner ? "In hoek" : "Groot"
                    onClicked: root.ov.timerCorner = !root.ov.timerCorner }
            }
        }
    }

    // ---- messages ---------------------------------------------------------------------
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 8

        RowLayout {
            Layout.fillWidth: true; spacing: 6
            Text { text: "Meldingen"; color: T.text; font.pixelSize: 15; font.bold: true }
            Item { Layout.fillWidth: true }
            AppButton { text: "Klok"; implicitHeight: 32; active: root.ov.clockShown; onClicked: root.ov.clockShown = !root.ov.clockShown }
            AppButton { text: "Nieuwe melding…"; implicitHeight: 32
                onClicked: { root.editIndex = -1; prompt.heading = "Nieuwe melding"; prompt.openWith("") } }
        }

        ListView {
            id: msgList
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; spacing: 4
            model: root.ov.messages
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                readonly property bool live: root.ov.messageShown && root.ov.messageText === modelData
                width: msgList.width - 8; height: 46; radius: 8
                color: live ? T.accentSoft : T.card; border.color: live ? T.accent : "transparent"
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 6; spacing: 6
                    Text { Layout.fillWidth: true; text: modelData; color: T.text; font.pixelSize: 13; elide: Text.ElideRight }
                    AppButton { text: live ? "Verberg" : "Toon"; primary: !live; implicitHeight: 32; implicitWidth: 80; leftPadding: 0; rightPadding: 0
                        onClicked: root.ov.toggleMessage(index) }
                    AppButton { text: "✎"; flat: true; implicitWidth: 32; implicitHeight: 32; leftPadding: 0; rightPadding: 0
                        onClicked: { root.editIndex = index; prompt.heading = "Melding bewerken"; prompt.openWith(modelData) } }
                    AppButton { text: "✕"; flat: true; implicitWidth: 32; implicitHeight: 32; leftPadding: 0; rightPadding: 0
                        onClicked: root.ov.removeMessage(index) }
                }
            }
        }
    }

    PromptDialog {
        id: prompt
        fieldLabel: "Tekst die onderaan het scherm komt"
        onConfirmed: function (text) {
            if (root.editIndex >= 0) root.ov.updateMessage(root.editIndex, text)
            else root.ov.addMessage(text)
        }
    }
}
