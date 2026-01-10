import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    property string tempFilePath: ""

    width: 600
    height: 400

    title: i18nc("@title:window", "Diktate")

    pageStack.initialPage: Kirigami.Page {
        id: page

        Layout.fillWidth: true
        Layout.fillHeight: true

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

            Connections {
                target: audioRecorder
                function onRecordingStarted() {
                    recordAction.text = i18n("Recording...")
                    recordAction.enabled = false
                }

                function onRecordingFinished(filePath) {
                    tempFilePath = filePath
                    recordAction.text = i18n("Transcribing...")
                    recordAction.enabled = false
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
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

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
                Layout.margins: 10

                Controls.TextArea {
                    id: textArea
                    placeholderText: i18n("Transcribed text will appear here...")
                    wrapMode: TextArea.Wrap
                    selectByMouse: true

                    MouseArea {
                        anchors.fill: parent
                        propagateComposedEvents: true
                        hoverEnabled: true

                        Controls.Button {
                            anchors {
                                top: parent.top
                                right: parent.right
                                margins: 10
                            }
                            text: i18n("Copy")
                            visible: parent.containsMouse
                            onClicked: {
                                textArea.selectAll()
                                textArea.copy()
                                textArea.deselect()
                                successMessage.visible = true
                                successMessageTimer.restart()
                            }
                        }
                    }
                }
            }
        }
    }
}
