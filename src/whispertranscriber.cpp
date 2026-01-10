#include "whispertranscriber.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <KLocalizedString>

WhisperTranscriber::WhisperTranscriber(QObject *parent)
    : QObject(parent)
{
    loadModel();
}

WhisperTranscriber::~WhisperTranscriber()
{
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

QString WhisperTranscriber::loadModel()
{
    QString modelPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(modelPath);
    modelPath += "/ggml-tiny.en.bin";

    if (!QFile::exists(modelPath)) {
        Q_EMIT errorOccurred(i18n("Whisper model not found at: %1\nPlease download from https://huggingface.co/ggerganov/whisper.cpp", modelPath));
        return QString();
    }

    struct whisper_model_params params = whisper_model_default_params();
    m_ctx = whisper_init_from_file(modelPath.toUtf8().constData(), params);

    if (!m_ctx) {
        Q_EMIT errorOccurred(i18n("Failed to load Whisper model"));
        return QString();
    }

    m_modelLoaded = true;
    return modelPath;
}

void WhisperTranscriber::transcribe(const QString &audioPath)
{
    if (!m_modelLoaded) {
        Q_EMIT errorOccurred(i18n("Whisper model not loaded"));
        return;
    }

    Q_EMIT transcriptionProgress(i18n("Transcribing..."));

    QThread *thread = QThread::create([this, audioPath]() {
        runTranscription(audioPath);
    });

    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void WhisperTranscriber::runTranscription(const QString &audioPath)
{
    QFile audioFile(audioPath);
    if (!audioFile.open(QIODevice::ReadOnly)) {
        Q_EMIT errorOccurred(i18n("Failed to open audio file"));
        return;
    }

    QByteArray audioData = audioFile.readAll();
    audioFile.close();

    if (audioData.isEmpty()) {
        Q_EMIT errorOccurred(i18n("Audio file is empty"));
        return;
    }

    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_special = false;
    params.print_realtime = false;
    params.translate = false;
    params.language = "en";
    params.n_threads = 4;
    params.offset_ms = 0;
    params.duration_ms = 0;

    if (whisper_full(m_ctx, params,
                     reinterpret_cast<const float*>(audioData.constData()),
                     audioData.size() / sizeof(float)) != 0) {
        Q_EMIT errorOccurred(i18n("Failed to transcribe audio"));
        return;
    }

    QString result;
    const int n_segments = whisper_full_n_segments(m_ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char *text = whisper_full_get_segment_text(m_ctx, i);
        result += QString::fromUtf8(text);
    }

    Q_EMIT transcriptionComplete(result.trimmed());
}

bool WhisperTranscriber::isModelLoaded() const
{
    return m_modelLoaded;
}
