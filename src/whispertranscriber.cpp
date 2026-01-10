#include "whispertranscriber.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QtConcurrent>
#include <KLocalizedString>

WhisperTranscriber::WhisperTranscriber(QObject *parent)
    : QObject(parent)
{
    loadModel();
}

WhisperTranscriber::~WhisperTranscriber()
{
    QMutexLocker locker(&m_mutex);
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

QString WhisperTranscriber::loadModel()
{
    QString modelPath = QDir::homePath() + QStringLiteral("/.local/share/diktate/ggml-tiny.en.bin");
    QDir dir(QDir::homePath() + QStringLiteral("/.local/share/diktate"));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    if (!QFile::exists(modelPath)) {
        Q_EMIT errorOccurred(i18n("Whisper model not found at: %1\nPlease download from https://huggingface.co/ggerganov/whisper.cpp").arg(modelPath));
        return QString();
    }

    struct whisper_context_params params = whisper_context_default_params();
    {
        QMutexLocker locker(&m_mutex);
        m_ctx = whisper_init_from_file_with_params(modelPath.toUtf8().constData(), params);
    }

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

    QtConcurrent::run([this, audioPath]() {
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

        // Convert Int16 samples to normalized float [-1.0, 1.0]
        const qint16 *samples = reinterpret_cast<const qint16 *>(audioData.constData());
        int sampleCount = audioData.size() / sizeof(qint16);

        std::vector<float> floatSamples(sampleCount);
        for (int i = 0; i < sampleCount; ++i) {
            floatSamples[i] = static_cast<float>(samples[i]) / 32768.0f;
        }

        struct whisper_context *ctx;
        {
            QMutexLocker locker(&m_mutex);
            ctx = m_ctx;
        }

        if (!ctx) {
            Q_EMIT errorOccurred(i18n("Whisper context not available"));
            return;
        }

        struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        params.print_progress = false;
        params.print_special = false;
        params.print_realtime = false;
        params.translate = false;
        params.language = "en";
        params.n_threads = 4;

        if (whisper_full(ctx, params, floatSamples.data(), sampleCount) != 0) {
            Q_EMIT errorOccurred(i18n("Failed to transcribe audio"));
            return;
        }

        QString result;
        const int n_segments = whisper_full_n_segments(ctx);
        for (int i = 0; i < n_segments; ++i) {
            const char *text = whisper_full_get_segment_text(ctx, i);
            result += QString::fromUtf8(text);
        }

        // Remove Whisper special tokens
        result.remove(QStringLiteral("[BLANK_AUDIO]"));

        Q_EMIT transcriptionComplete(result.trimmed());
    });
}

bool WhisperTranscriber::isModelLoaded() const
{
    return m_modelLoaded;
}
