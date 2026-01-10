#include "whispertranscriber.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QtConcurrent>
#include <KLocalizedString>
#include <thread>
#include <algorithm>

WhisperTranscriber::WhisperTranscriber(QObject *parent)
    : QObject(parent)
{
}

WhisperTranscriber::~WhisperTranscriber()
{
    QMutexLocker locker(&m_mutex);
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

void WhisperTranscriber::loadModel(const QString &modelPath)
{
    if (modelPath.isEmpty()) {
        return;
    }

    if (!QFile::exists(modelPath)) {
        Q_EMIT errorOccurred(i18n("Model file not found: %1", modelPath));
        return;
    }

    // If already loading the same model, skip
    if (m_modelLoaded && m_currentModelPath == modelPath) {
        return;
    }

    Q_EMIT transcriptionProgress(i18n("Loading model..."));

    // Free existing context if any
    {
        QMutexLocker locker(&m_mutex);
        if (m_ctx) {
            whisper_free(m_ctx);
            m_ctx = nullptr;
        }
        m_modelLoaded = false;
    }

    struct whisper_context_params params = whisper_context_default_params();
    params.use_gpu = true;  // Enable GPU acceleration if available
    params.flash_attn = true;  // Enable Flash Attention for better performance
    struct whisper_context *newCtx = whisper_init_from_file_with_params(modelPath.toUtf8().constData(), params);

    if (!newCtx) {
        Q_EMIT errorOccurred(i18n("Failed to load Whisper model: %1", modelPath));
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_ctx = newCtx;
        m_modelLoaded = true;
        m_currentModelPath = modelPath;
    }

    Q_EMIT modelLoaded(modelPath);
    Q_EMIT transcriptionProgress(i18n("Model loaded"));
}

QString WhisperTranscriber::currentModelPath() const
{
    return m_currentModelPath;
}

void WhisperTranscriber::transcribe(const QString &audioPath)
{
    if (!m_modelLoaded) {
        Q_EMIT errorOccurred(i18n("No model loaded. Please select and download a model first."));
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
        params.language = "auto";  // Use auto-detect for multilingual support

        // Dynamic thread count: use all available cores minus one for responsiveness
        const int maxThreads = static_cast<int>(std::thread::hardware_concurrency());
        params.n_threads = std::max(1, maxThreads > 1 ? maxThreads - 1 : maxThreads);

        // Dynamic audio context optimization for short clips (from whisper.cpp issue #1855)
        // This significantly speeds up transcription of short recordings
        constexpr int sampleRate = 16000;
        constexpr int maxAudioCtx = 1500;
        const int dynamicAudioCtx = std::min(maxAudioCtx,
            ((maxAudioCtx * sampleCount) / (sampleRate * 30)) + 128);
        params.audio_ctx = dynamicAudioCtx;

        // Suppress blank and non-speech tokens for cleaner output
        params.suppress_blank = true;
        params.suppress_non_speech_tokens = true;

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
