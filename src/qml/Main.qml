import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: 600
    height: 400

    title: i18nc("@title:window", "Diktate")

    pageStack.initialPage: Kirigami.Page {
        id: page

        title: i18n("Diktate")

        Layout.fillWidth: true
        Layout.fillHeight: true

        property string tempFilePath: ""

        actions: Kirigami.Action {
            id: recordAction
            text: i18n("Record")
            icon.name: "media-record"
            onTriggered: {
                if (audioRecorder.isRecording()) {
                    audioRecorder.stopRecording();
                    transcriber.transcribe(tempFilePath);
                } else {
                    audioRecorder.startRecording();
                }
            }
        }

        Connections {
            target: audioRecorder
            function onRecordingStarted() {
                recordAction.text = i18n("Recording...")
                recordAction.enabled = false
            }

            function onRecordingFinished(filePath) {
                page.tempFilePath = filePath
                recordAction.text = i18n("Transcribing...")
                recordAction.enabled = false
                transcriber.transcribe(filePath)
            }

            function onErrorOccurred(message) {
                errorInlineMessage.text = message
                errorInlineMessage.visible = true
                recordAction.text = i18n("Record")
                recordAction.enabled = true
            }
        }

        Connections {
            target: transcriber
            function onTranscriptionComplete(text) {
                textArea.text = text
                recordAction.text = i18n("Record")
                recordAction.enabled = true
                successMessage.visible = true
                successMessageTimer.restart()
            }

            function onTranscriptionProgress(status) {
                statusLabel.text = status
            }

            function onErrorOccurred(message) {
                errorInlineMessage.text = message
                errorInlineMessage.visible = true
                recordAction.text = i18n("Record")
                recordAction.enabled = true
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.InlineMessage {
                id: errorInlineMessage
                Layout.fillWidth: true
                type: Kirigami.MessageType.Error
                visible: false

                Connections {
                    target: audioRecorder
                    function onRecordingStarted() {
                        errorInlineMessage.visible = false
                    }
                }
            }

            Kirigami.InlineMessage {
                id: successMessage
                Layout.fillWidth: true
                type: Kirigami.MessageType.Positive
                text: i18n("Text copied")
                visible: false

                Timer {
                    id: successMessageTimer
                    interval: 2000
                    onTriggered: successMessage.visible = false
                }
            }

            Controls.Label {
                id: statusLabel
                Layout.alignment: Qt.AlignHCenter
                text: i18n("Press Record to start...")
                font.italic: true
                color: Kirigami.Theme.disabledTextColor
            }

            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Kirigami.Units.largeSpacing

                Controls.TextArea {
                    id: textArea
                    placeholderText: i18n("Transcribed text will appear here...")
                    wrapMode: Controls.TextArea.Wrap
                    selectByMouse: true

                    HoverHandler {
                        id: textAreaHover
                    }

                    Controls.ToolButton {
                        anchors {
                            top: parent.top
                            right: parent.right
                            margins: Kirigami.Units.smallSpacing
                        }
                        icon.name: "edit-copy"
                        text: i18n("Copy")
                        display: Controls.AbstractButton.IconOnly
                        visible: textAreaHover.hovered && textArea.text.length > 0

                        Controls.ToolTip.text: text
                        Controls.ToolTip.visible: hovered
                        Controls.ToolTip.delay: Kirigami.Units.toolTipDelay

                        onClicked: {
                            textArea.selectAll()
                            textArea.copy()
                            textArea.deselect()
                            successMessage.text = i18n("Copied to clipboard")
                            successMessage.visible = true
                            successMessageTimer.restart()
                        }
                    }
                }
            }
        }
    }
}
