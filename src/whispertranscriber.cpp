#include "whispertranscriber.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QtConcurrent>
#include <KLocalizedString>
#include <thread>
#include <algorithm>

// Static map of Whisper language codes to display names
// Based on OpenAI Whisper's official language list
const QMap<QString, QString> WhisperTranscriber::s_languageNames = {
    {QStringLiteral("auto"), QStringLiteral("Auto Detect")},
    {QStringLiteral("en"), QStringLiteral("English")},
    {QStringLiteral("zh"), QStringLiteral("Chinese")},
    {QStringLiteral("de"), QStringLiteral("German")},
    {QStringLiteral("es"), QStringLiteral("Spanish")},
    {QStringLiteral("ru"), QStringLiteral("Russian")},
    {QStringLiteral("ko"), QStringLiteral("Korean")},
    {QStringLiteral("fr"), QStringLiteral("French")},
    {QStringLiteral("ja"), QStringLiteral("Japanese")},
    {QStringLiteral("pt"), QStringLiteral("Portuguese")},
    {QStringLiteral("tr"), QStringLiteral("Turkish")},
    {QStringLiteral("pl"), QStringLiteral("Polish")},
    {QStringLiteral("ca"), QStringLiteral("Catalan")},
    {QStringLiteral("nl"), QStringLiteral("Dutch")},
    {QStringLiteral("ar"), QStringLiteral("Arabic")},
    {QStringLiteral("sv"), QStringLiteral("Swedish")},
    {QStringLiteral("it"), QStringLiteral("Italian")},
    {QStringLiteral("id"), QStringLiteral("Indonesian")},
    {QStringLiteral("hi"), QStringLiteral("Hindi")},
    {QStringLiteral("fi"), QStringLiteral("Finnish")},
    {QStringLiteral("vi"), QStringLiteral("Vietnamese")},
    {QStringLiteral("he"), QStringLiteral("Hebrew")},
    {QStringLiteral("uk"), QStringLiteral("Ukrainian")},
    {QStringLiteral("el"), QStringLiteral("Greek")},
    {QStringLiteral("ms"), QStringLiteral("Malay")},
    {QStringLiteral("cs"), QStringLiteral("Czech")},
    {QStringLiteral("ro"), QStringLiteral("Romanian")},
    {QStringLiteral("da"), QStringLiteral("Danish")},
    {QStringLiteral("hu"), QStringLiteral("Hungarian")},
    {QStringLiteral("ta"), QStringLiteral("Tamil")},
    {QStringLiteral("no"), QStringLiteral("Norwegian")},
    {QStringLiteral("th"), QStringLiteral("Thai")},
    {QStringLiteral("ur"), QStringLiteral("Urdu")},
    {QStringLiteral("hr"), QStringLiteral("Croatian")},
    {QStringLiteral("bg"), QStringLiteral("Bulgarian")},
    {QStringLiteral("lt"), QStringLiteral("Lithuanian")},
    {QStringLiteral("la"), QStringLiteral("Latin")},
    {QStringLiteral("mi"), QStringLiteral("Maori")},
    {QStringLiteral("ml"), QStringLiteral("Malayalam")},
    {QStringLiteral("cy"), QStringLiteral("Welsh")},
    {QStringLiteral("sk"), QStringLiteral("Slovak")},
    {QStringLiteral("te"), QStringLiteral("Telugu")},
    {QStringLiteral("fa"), QStringLiteral("Persian")},
    {QStringLiteral("lv"), QStringLiteral("Latvian")},
    {QStringLiteral("bn"), QStringLiteral("Bengali")},
    {QStringLiteral("sr"), QStringLiteral("Serbian")},
    {QStringLiteral("az"), QStringLiteral("Azerbaijani")},
    {QStringLiteral("sl"), QStringLiteral("Slovenian")},
    {QStringLiteral("kn"), QStringLiteral("Kannada")},
    {QStringLiteral("et"), QStringLiteral("Estonian")},
    {QStringLiteral("mk"), QStringLiteral("Macedonian")},
    {QStringLiteral("br"), QStringLiteral("Breton")},
    {QStringLiteral("eu"), QStringLiteral("Basque")},
    {QStringLiteral("is"), QStringLiteral("Icelandic")},
    {QStringLiteral("hy"), QStringLiteral("Armenian")},
    {QStringLiteral("ne"), QStringLiteral("Nepali")},
    {QStringLiteral("mn"), QStringLiteral("Mongolian")},
    {QStringLiteral("bs"), QStringLiteral("Bosnian")},
    {QStringLiteral("kk"), QStringLiteral("Kazakh")},
    {QStringLiteral("sq"), QStringLiteral("Albanian")},
    {QStringLiteral("sw"), QStringLiteral("Swahili")},
    {QStringLiteral("gl"), QStringLiteral("Galician")},
    {QStringLiteral("mr"), QStringLiteral("Marathi")},
    {QStringLiteral("pa"), QStringLiteral("Punjabi")},
    {QStringLiteral("si"), QStringLiteral("Sinhala")},
    {QStringLiteral("km"), QStringLiteral("Khmer")},
    {QStringLiteral("sn"), QStringLiteral("Shona")},
    {QStringLiteral("yo"), QStringLiteral("Yoruba")},
    {QStringLiteral("so"), QStringLiteral("Somali")},
    {QStringLiteral("af"), QStringLiteral("Afrikaans")},
    {QStringLiteral("oc"), QStringLiteral("Occitan")},
    {QStringLiteral("ka"), QStringLiteral("Georgian")},
    {QStringLiteral("be"), QStringLiteral("Belarusian")},
    {QStringLiteral("tg"), QStringLiteral("Tajik")},
    {QStringLiteral("sd"), QStringLiteral("Sindhi")},
    {QStringLiteral("gu"), QStringLiteral("Gujarati")},
    {QStringLiteral("am"), QStringLiteral("Amharic")},
    {QStringLiteral("yi"), QStringLiteral("Yiddish")},
    {QStringLiteral("lo"), QStringLiteral("Lao")},
    {QStringLiteral("uz"), QStringLiteral("Uzbek")},
    {QStringLiteral("fo"), QStringLiteral("Faroese")},
    {QStringLiteral("ht"), QStringLiteral("Haitian Creole")},
    {QStringLiteral("ps"), QStringLiteral("Pashto")},
    {QStringLiteral("tk"), QStringLiteral("Turkmen")},
    {QStringLiteral("nn"), QStringLiteral("Nynorsk")},
    {QStringLiteral("mt"), QStringLiteral("Maltese")},
    {QStringLiteral("sa"), QStringLiteral("Sanskrit")},
    {QStringLiteral("lb"), QStringLiteral("Luxembourgish")},
    {QStringLiteral("my"), QStringLiteral("Myanmar")},
    {QStringLiteral("bo"), QStringLiteral("Tibetan")},
    {QStringLiteral("tl"), QStringLiteral("Tagalog")},
    {QStringLiteral("mg"), QStringLiteral("Malagasy")},
    {QStringLiteral("as"), QStringLiteral("Assamese")},
    {QStringLiteral("tt"), QStringLiteral("Tatar")},
    {QStringLiteral("haw"), QStringLiteral("Hawaiian")},
    {QStringLiteral("ln"), QStringLiteral("Lingala")},
    {QStringLiteral("ha"), QStringLiteral("Hausa")},
    {QStringLiteral("ba"), QStringLiteral("Bashkir")},
    {QStringLiteral("jw"), QStringLiteral("Javanese")},
    {QStringLiteral("su"), QStringLiteral("Sundanese")},
    {QStringLiteral("yue"), QStringLiteral("Cantonese")},
};

WhisperTranscriber::WhisperTranscriber(QObject *parent)
    : QObject(parent)
{
    // Load saved settings
    QSettings settings;
    m_useGpu = settings.value(QStringLiteral("transcriber/useGpu"), true).toBool();
    m_gpuDevice = settings.value(QStringLiteral("transcriber/gpuDevice"), 0).toInt();
    m_language = settings.value(QStringLiteral("transcriber/language"), QStringLiteral("auto")).toString();
}

WhisperTranscriber::~WhisperTranscriber()
{
    QMutexLocker locker(&m_mutex);
    if (m_ctx) {
        whisper_free(m_ctx);
        m_ctx = nullptr;
    }
}

bool WhisperTranscriber::useGpu() const
{
    return m_useGpu;
}

void WhisperTranscriber::setUseGpu(bool enabled)
{
    if (m_useGpu != enabled) {
        m_useGpu = enabled;

        // Save setting
        QSettings settings;
        settings.setValue(QStringLiteral("transcriber/useGpu"), m_useGpu);

        Q_EMIT useGpuChanged();

        // Reload model with new GPU settings
        reloadModelIfNeeded();
    }
}

int WhisperTranscriber::gpuDevice() const
{
    return m_gpuDevice;
}

void WhisperTranscriber::setGpuDevice(int device)
{
    if (m_gpuDevice != device) {
        m_gpuDevice = device;

        // Save setting
        QSettings settings;
        settings.setValue(QStringLiteral("transcriber/gpuDevice"), m_gpuDevice);

        Q_EMIT gpuDeviceChanged();

        // Reload model with new GPU device
        if (m_useGpu) {
            reloadModelIfNeeded();
        }
    }
}

void WhisperTranscriber::reloadModelIfNeeded()
{
    if (m_modelLoaded && !m_currentModelPath.isEmpty()) {
        QString modelPath = m_currentModelPath;
        m_currentModelPath.clear();  // Force reload
        m_modelLoaded = false;
        loadModel(modelPath);
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
    params.use_gpu = m_useGpu;
    params.gpu_device = m_gpuDevice;
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

    // Capture the language setting for the async lambda
    const QString languageCode = m_language;

    QtConcurrent::run([this, audioPath, languageCode]() {
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

        // Set the language for transcription
        // "auto" means auto-detect, otherwise use the specified language code
        QByteArray langBytes = languageCode.toUtf8();
        params.language = langBytes.constData();

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

QString WhisperTranscriber::language() const
{
    return m_language;
}

void WhisperTranscriber::setLanguage(const QString &language)
{
    if (m_language != language) {
        m_language = language;

        // Save setting
        QSettings settings;
        settings.setValue(QStringLiteral("transcriber/language"), m_language);

        Q_EMIT languageChanged();
    }
}

QStringList WhisperTranscriber::supportedLanguages() const
{
    // Return language codes in a specific order:
    // 1. "auto" for auto-detect
    // 2. Rest alphabetically by display name
    QStringList codes = s_languageNames.keys();

    // Sort by display name, but keep "auto" first
    std::sort(codes.begin(), codes.end(), [this](const QString &a, const QString &b) {
        if (a == QStringLiteral("auto")) return true;
        if (b == QStringLiteral("auto")) return false;
        return s_languageNames.value(a) < s_languageNames.value(b);
    });

    return codes;
}

QString WhisperTranscriber::languageDisplayName(const QString &code) const
{
    return s_languageNames.value(code, code);
}
