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

    // Reset VAD and segmentation state for new recording
    if (m_vad && m_vad->isValid()) {
        m_vad->reset();
    }
    resetSegmentation();

    Q_EMIT recordingStarted();
}

void AudioRecorder::stopRecording()
{
    if (!m_isRecording) {
        return;
    }

    m_silenceTimer->stop();

    // Flush any in-progress speech segment so it gets transcribed
    if (m_vad && m_vad->isValid()) {
        emitSegment(true);
    } else if (m_hasDetectedVoice && m_audioData.size() >= MIN_SEGMENT_BYTES) {
        Q_EMIT segmentReady(m_audioData);
    }

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
    if (!m_vad || !m_vad->isValid()) {
        // Without VAD there is no segmentation; the whole buffer is emitted
        // as a single segment when recording stops.
        qint16 rms = calculateRMS(data);
        if (rms > SILENCE_THRESHOLD_RMS) {
            m_hasDetectedVoice = true;
            m_lastVoiceTime.restart();
        }
        return;
    }

    m_pendingBytes.append(data);
    while (m_pendingBytes.size() >= FRAME_BYTES) {
        QByteArray frame = m_pendingBytes.left(FRAME_BYTES);
        m_pendingBytes.remove(0, FRAME_BYTES);
        processFrame(frame);
    }
}

void AudioRecorder::processFrame(const QByteArray &frame)
{
    // Feeding exactly one 10ms frame makes the returned state correspond
    // to this frame (VAD applies hysteresis internally).
    const bool speech = m_vad->isSpeech(frame);

    if (speech) {
        m_hasDetectedVoice = true;
        m_lastVoiceTime.restart();
    }

    if (!m_inSegment) {
        // Keep a small pre-roll so word onsets are not clipped
        m_preRoll.append(frame);
        if (m_preRoll.size() > PRE_ROLL_BYTES) {
            m_preRoll.remove(0, m_preRoll.size() - PRE_ROLL_BYTES);
        }

        if (speech) {
            m_inSegment = true;
            m_segment = m_preRoll;
            m_preRoll.clear();
        }
        return;
    }

    m_segment.append(frame);

    if (speech) {
        if (m_segment.size() >= MAX_SEGMENT_BYTES) {
            // Safety cap: split long uninterrupted speech so transcription
            // of this chunk can start while recording continues.
            emitSegment(false);
            // Stay in segment mode so following frames start a fresh
            // segment immediately (no pre-roll, avoiding duplicated audio).
            m_inSegment = true;
        }
    } else {
        // VAD flipped to silence: a natural pause ended the sentence
        emitSegment(true);
    }
}

void AudioRecorder::emitSegment(bool trimTrailingSilence)
{
    if (!m_inSegment) {
        return;
    }

    m_inSegment = false;

    if (trimTrailingSilence) {
        // The VAD silence hysteresis already included trailing silence in the
        // segment; trim it down to a small natural padding.
        const int hysteresisBytes = VAD::silenceHysteresisMs() * 32; // 32 bytes/ms at 16kHz int16 mono
        const int trimBytes = std::max(0, hysteresisBytes - TAIL_PADDING_BYTES);
        m_segment.chop(std::min(trimBytes, static_cast<int>(m_segment.size())));
    }

    if (m_segment.size() >= MIN_SEGMENT_BYTES) {
        Q_EMIT segmentReady(m_segment);
    }
    m_segment.clear();
}

void AudioRecorder::resetSegmentation()
{
    m_pendingBytes.clear();
    m_preRoll.clear();
    m_segment.clear();
    m_inSegment = false;
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
    resetSegmentation();

    // Reset VAD state
    if (m_vad && m_vad->isValid()) {
        m_vad->reset();
    }
}
