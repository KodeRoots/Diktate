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
                textArea.placeholderText = i18n("Transcribing...")
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
                textArea.placeholderText = i18n("Press Record to start...")
                recordAction.text = i18n("Record")
                recordAction.enabled = true
                successMessage.text = i18n("Transcription complete")
                successMessage.visible = true
                successMessageTimer.restart()
            }

            function onTranscriptionProgress(status) {
                textArea.placeholderText = status
            }

            function onErrorOccurred(message) {
                textArea.placeholderText = i18n("Press Record to start...")
                errorInlineMessage.text = message
                errorInlineMessage.visible = true
                recordAction.text = i18n("Record")
                recordAction.enabled = true
            }
        }

        // Main content
        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.mediumSpacing

            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Kirigami.Units.largeSpacing

                Controls.TextArea {
                    id: textArea
                    placeholderText: i18n("Press Record to start...")
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

        // Floating notifications overlay
        ColumnLayout {
            anchors {
                bottom: parent.bottom
                left: parent.left
                right: parent.right
                margins: Kirigami.Units.largeSpacing
            }
            spacing: Kirigami.Units.smallSpacing
            z: 1

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
        }
    }
}
