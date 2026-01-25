#include "audiorecorder.h"
#include "vad.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QDebug>
#include <KLocalizedString>
#include <QtMath>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent)
{
    // Initialize WebRTC VAD for industrial-grade voice activity detection
    // Uses 16kHz sample rate (required by Whisper) and VeryAggressive mode
    // VeryAggressive is the most strict - better at rejecting background noise
    m_vad = std::make_unique<VAD>(16000, VAD::Mode::VeryAggressive);
    
    if (m_vad->isValid()) {
        qDebug() << "AudioRecorder: WebRTC VAD initialized successfully";
    } else {
        qWarning() << "AudioRecorder: VAD initialization failed, falling back to RMS threshold";
    }

    m_silenceTimer = new QTimer(this);
    m_silenceTimer->setInterval(100); // Check every 100ms
    connect(m_silenceTimer, &QTimer::timeout, this, [this]() {
        if (m_hasDetectedVoice && m_lastVoiceTime.elapsed() > SILENCE_DURATION_MS) {
            stopRecording();
        }
    });
}

AudioRecorder::~AudioRecorder()
{
    cleanup();
}

void AudioRecorder::startRecording()
{
    if (m_isRecording) {
        return;
    }

    cleanup();

    QAudioFormat format;
    format.setSampleRate(16000);  // 16kHz required by Whisper
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        Q_EMIT errorOccurred(i18n("No audio input device found"));
        return;
    }

    m_audioSource = new QAudioSource(device, format, this);
    m_audioSource->setBufferSize(4096);

    m_tempFile = new QTemporaryFile(this);
    if (!m_tempFile->open()) {
        Q_EMIT errorOccurred(i18n("Failed to create temporary file"));
        cleanup();
        return;
    }

    m_audioDevice = m_audioSource->start();
    if (!m_audioDevice) {
        Q_EMIT errorOccurred(i18n("Failed to start audio capture"));
        cleanup();
        return;
    }

    connect(m_audioSource, &QAudioSource::stateChanged,
            this, [this](QAudio::State state) {
        if (state == QAudio::StoppedState && m_isRecording) {
            m_tempFile->write(m_audioData);
            m_tempFile->close();
            m_isRecording = false;
            Q_EMIT recordingFinished(m_tempFile->fileName());
        }
    });

    connect(m_audioDevice, &QIODevice::readyRead,
            this, [this]() {
        QByteArray data = m_audioDevice->readAll();
        m_audioData.append(data);
        processAudioData(data);
    });

    m_isRecording = true;
    m_hasDetectedVoice = false;
    m_lastVoiceTime.start();
    m_silenceTimer->start();
    
    // Reset VAD state for new recording
    if (m_vad && m_vad->isValid()) {
        m_vad->reset();
    }
    
    Q_EMIT recordingStarted();
}

void AudioRecorder::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_silenceTimer->stop();

    if (m_audioSource) {
        m_audioSource->stop();
    }

    m_isRecording = false;
}

bool AudioRecorder::isRecording() const
{
    return m_isRecording;
}

void AudioRecorder::processAudioData(const QByteArray &data)
{
    bool voiceDetected = false;
    
    // Use WebRTC VAD if available (more accurate than RMS)
    if (m_vad && m_vad->isValid()) {
        voiceDetected = m_vad->isSpeech(data);
    } else {
        // Fallback to RMS threshold if VAD not available
        qint16 rms = calculateRMS(data);
        voiceDetected = (rms > SILENCE_THRESHOLD_RMS);
    }

    if (voiceDetected) {
        m_hasDetectedVoice = true;
        m_lastVoiceTime.restart();
    }
}

qint16 AudioRecorder::calculateRMS(const QByteArray &data)
{
    if (data.size() < 2) {
        return 0;
    }

    const qint16 *samples = reinterpret_cast<const qint16 *>(data.constData());
    int sampleCount = data.size() / sizeof(qint16);

    qint64 sumSquares = 0;
    for (int i = 0; i < sampleCount; ++i) {
        sumSquares += static_cast<qint64>(samples[i]) * samples[i];
    }

    return static_cast<qint16>(qSqrt(sumSquares / sampleCount));
}

void AudioRecorder::cleanup()
{
    m_isRecording = false;
    m_hasDetectedVoice = false;

    if (m_silenceTimer) {
        m_silenceTimer->stop();
    }

    if (m_audioSource) {
        delete m_audioSource;
        m_audioSource = nullptr;
    }

    m_audioDevice = nullptr;

    if (m_tempFile) {
        delete m_tempFile;
        m_tempFile = nullptr;
    }

    m_audioData.clear();
    
    // Reset VAD state
    if (m_vad && m_vad->isValid()) {
        m_vad->reset();
    }
}
