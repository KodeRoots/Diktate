#include "audiorecorder.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QDebug>
#include <KLocalizedString>

AudioRecorder::AudioRecorder(QObject *parent)
    : QObject(parent)
{
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
            Q_EMIT recordingFinished(m_tempFile->fileName());
        }
    });

    connect(m_audioDevice, &QIODevice::readyRead,
            this, [this]() {
        m_audioData.append(m_audioDevice->readAll());
    });

    m_isRecording = true;
    Q_EMIT recordingStarted();
}

void AudioRecorder::stopRecording()
{
    if (!m_isRecording) {
        return;
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

void AudioRecorder::cleanup()
{
    m_isRecording = false;

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
}
