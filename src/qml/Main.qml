import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import Qt.labs.platform as Platform
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: 680
    height: 400

    minimumWidth: 680
    minimumHeight: 400

    title: i18nc("@title:window", "Diktate")

    onClosing: function(close) {
        close.accepted = false
        systemTray.visible = false
    }

    // Language data - codes map to display names by index
    // Built once on component completion
    property var languageDisplayNames: []
    property var languageCodes: []

    Component.onCompleted: {
        buildLanguageModel();
    }

    function buildLanguageModel() {
        let names = [];
        let codes = [];

        // First: English (uses English-only model) - special code "en-model"
        names.push(i18n("English (Optimized)"));
        codes.push("en-model");

        // Second: Auto Detect (uses Multilingual model)
        names.push(i18n("Auto Detect"));
        codes.push("auto");

        // Get all supported languages and sort them alphabetically
        let languages = transcriber.supportedLanguages;
        let langList = [];

        for (let i = 0; i < languages.length; i++) {
            let code = languages[i];
            // Skip "auto" as we already added it above
            if (code === "auto")
                continue;
            let displayName = transcriber.languageDisplayName(code);
            langList.push({
                code: code,
                displayName: displayName
            });
        }

        // Sort alphabetically by display name
        langList.sort((a, b) => a.displayName.localeCompare(b.displayName));

        // Add sorted languages
        for (let j = 0; j < langList.length; j++) {
            names.push(langList[j].displayName);
            codes.push(langList[j].code);
        }

        languageDisplayNames = names;
        languageCodes = codes;
    }

    // Helper function to find language index by code
    function findLanguageIndex(code) {
        for (let i = 0; i < languageCodes.length; i++) {
            if (languageCodes[i] === code) {
                return i;
            }
        }
        return 1;  // Default to "Auto Detect"
    }

    // Helper function to get initial language index based on current settings
    function getInitialLanguageIndex() {
        // If using English-only model, select "English (Optimized)"
        if (modelManager.modelType === 0) {  // EnglishOnly
            return 0;  // "en-model"
        }
        // Otherwise, find the transcriber's language in the list
        return findLanguageIndex(transcriber.language);
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        title: i18nc("@title:drawer", "Diktate")
        titleIcon: "diktate"
        isMenu: true
        actions: [
            Kirigami.Action {
                id: gpuAction
                text: i18n("GPU Acceleration")
                icon.name: "show-gpu-effects-symbolic"
                checkable: true
                checked: transcriber.useGpu
                visible: gpuInfo.gpuCount > 0
                onToggled: {
                    transcriber.useGpu = checked;
                    if (checked) {
                        root.showPassiveNotification(i18n("GPU acceleration enabled. Model will reload."), "short");
                    } else {
                        root.showPassiveNotification(i18n("GPU acceleration disabled. Model will reload."), "short");
                    }
                }
            }
        ]
    }

    pageStack.initialPage: Kirigami.Page {
        id: page

        title: i18n("Diktate")

        Layout.fillWidth: true
        Layout.fillHeight: true

        property string tempFilePath: ""

        actions: [
            Kirigami.Action {
                id: uploadAction
                text: i18n("Upload File")
                icon.name: "document-open"
                enabled: transcriber.isModelLoaded() && !audioFileProcessor.isProcessing
                displayHint: Kirigami.DisplayHint.KeepVisible
                onTriggered: fileDialog.open()
            },
            Kirigami.Action {
                id: recordAction
                text: i18n("Record")
                icon.name: "media-record"
                enabled: transcriber.isModelLoaded() && !audioFileProcessor.isProcessing
                displayHint: Kirigami.DisplayHint.KeepVisible
                onTriggered: {
                    if (audioRecorder.isRecording()) {
                        audioRecorder.stopRecording();
                        transcriber.transcribe(tempFilePath);
                    } else {
                        audioRecorder.startRecording();
                    }
                }
            }
        ]

        Connections {
            target: audioRecorder
            function onRecordingStarted() {
                recordAction.text = i18n("Recording...");
                recordAction.enabled = false;
                uploadAction.enabled = false;
            }

            function onRecordingFinished(filePath) {
                page.tempFilePath = filePath;
                recordAction.text = i18n("Transcribing...");
                recordAction.enabled = false;
                uploadAction.enabled = false;
                textArea.placeholderText = i18n("Transcribing...");
                transcriber.transcribe(filePath);
            }

            function onErrorOccurred(message) {
                errorInlineMessage.text = message;
                errorInlineMessage.visible = true;
                recordAction.text = i18n("Record");
                recordAction.enabled = transcriber.isModelLoaded();
                uploadAction.enabled = transcriber.isModelLoaded();
            }
        }

        Connections {
            target: transcriber
            function onTranscriptionComplete(text) {
                textArea.text = text;
                textArea.placeholderText = i18n("Press Record to start...");
                recordAction.text = i18n("Record");
                recordAction.enabled = true;
                uploadAction.enabled = true;
                successMessage.text = i18n("Transcription complete");
                successMessage.visible = true;
                successMessageTimer.restart();
            }

            function onTranscriptionProgress(status) {
                textArea.placeholderText = status;
            }

            function onErrorOccurred(message) {
                textArea.placeholderText = i18n("Press Record to start...");
                errorInlineMessage.text = message;
                errorInlineMessage.visible = true;
                recordAction.text = i18n("Record");
                recordAction.enabled = transcriber.isModelLoaded();
                uploadAction.enabled = transcriber.isModelLoaded();
            }

            function onModelLoaded(modelPath) {
                recordAction.enabled = true;
                uploadAction.enabled = true;
                textArea.placeholderText = i18n("Press Record to start...");
            }
        }

        Connections {
            target: modelManager
            function onDownloadComplete() {
                successMessage.text = i18n("Model downloaded successfully");
                successMessage.visible = true;
                successMessageTimer.restart();
            }

            function onDownloadError(message) {
                errorInlineMessage.text = message;
                errorInlineMessage.visible = true;
            }
        }

        // Main content
        ColumnLayout {
            anchors.fill: parent
            spacing: Kirigami.Units.mediumSpacing

            // Model selection row
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                Layout.bottomMargin: 0
                spacing: Kirigami.Units.mediumSpacing

                Controls.Label {
                    text: i18n("Model:")
                }

                Controls.ComboBox {
                    id: modelSizeCombo
                    Layout.fillWidth: true
                    model: [i18n("Tiny"), i18n("Base"), i18n("Small"), i18n("Medium"), i18n("Large")]
                    currentIndex: modelManager.modelSize
                    enabled: !modelManager.isDownloading
                    onActivated: function (index) {
                        modelManager.modelSize = index;
                    }
                }

                Controls.Label {
                    text: i18n("Language:")
                }

                Controls.ComboBox {
                    id: languageCombo
                    Layout.fillWidth: true

                    model: languageDisplayNames
                    currentIndex: getInitialLanguageIndex()
                    enabled: !modelManager.isDownloading

                    onActivated: function (index) {
                        let code = languageCodes[index];

                        if (code === "en-model") {
                            // User selected English - use English-only model
                            modelManager.modelType = 0;  // EnglishOnly
                            transcriber.language = "en";
                        } else {
                            // User selected Auto or a specific language - use Multilingual model
                            modelManager.modelType = 1;  // Multilingual
                            transcriber.language = code;
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            // Download prompt message
            Kirigami.InlineMessage {
                id: downloadPromptMessage
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                type: Kirigami.MessageType.Information
                visible: !modelManager.isCurrentModelAvailable || modelManager.isDownloading
                text: {
                    if (modelManager.isDownloading) {
                        return i18n("Downloading %1... %2%", modelManager.getModelDisplayName(modelManager.modelSize, modelManager.modelType), Math.round(modelManager.downloadProgress));
                    } else {
                        return i18n("Model %1 is not installed", modelManager.getModelDisplayName(modelManager.modelSize, modelManager.modelType));
                    }
                }

                actions: [
                    Kirigami.Action {
                        text: modelManager.isDownloading ? i18n("Cancel") : i18n("Download and Install")
                        icon.name: modelManager.isDownloading ? "dialog-cancel" : "download"
                        onTriggered: {
                            if (modelManager.isDownloading) {
                                modelManager.cancelDownload();
                            } else {
                                modelManager.downloadCurrentModel();
                            }
                        }
                    }
                ]
            }

            Controls.ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Kirigami.Units.largeSpacing
                Layout.topMargin: downloadPromptMessage.visible ? 0 : Kirigami.Units.largeSpacing

                Controls.TextArea {
                    id: textArea
                    placeholderText: transcriber.isModelLoaded() ? i18n("Press Record to start...") : i18n("Select and download a model to start")
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
                            textArea.selectAll();
                            textArea.copy();
                            textArea.deselect();
                            successMessage.text = i18n("Copied to clipboard");
                            successMessage.visible = true;
                            successMessageTimer.restart();
                        }
                    }
                }
            }
        }

        Platform.FileDialog {
            id: fileDialog
            title: i18n("Select Audio File")
            nameFilters: [i18n("Audio Files (*.wav *.mp3 *.ogg *.flac *.m4a *.aac *.wma *.opus *.webm)"), i18n("All Files (*)")]
            fileMode: Platform.FileDialog.OpenFile
            onAccepted: {
                audioFileProcessor.processFile(fileDialog.currentFile);
                uploadAction.enabled = false;
                recordAction.enabled = false;
                textArea.placeholderText = i18n("Decoding audio file...");
            }
        }

        Connections {
            target: audioFileProcessor
            function onProcessingStarted() {
                uploadAction.text = i18n("Decoding...");
                uploadAction.enabled = false;
                recordAction.enabled = false;
                textArea.placeholderText = i18n("Decoding audio file...");
            }

            function onProcessingFinished(tempFilePath) {
                uploadAction.text = i18n("Upload File");
                recordAction.text = i18n("Transcribing...");
                recordAction.enabled = false;
                uploadAction.enabled = false;
                textArea.placeholderText = i18n("Transcribing...");
            }

            function onErrorOccurred(message) {
                errorInlineMessage.text = message;
                errorInlineMessage.visible = true;
                uploadAction.text = i18n("Upload File");
                uploadAction.enabled = transcriber.isModelLoaded();
                recordAction.text = i18n("Record");
                recordAction.enabled = transcriber.isModelLoaded();
                textArea.placeholderText = i18n("Press Record to start...");
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
                        errorInlineMessage.visible = false;
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
