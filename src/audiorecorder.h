#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QAudioSource>
#include <QAudioDevice>
#include <QBuffer>
#include <QFile>
#include <QTemporaryFile>
#include <QTimer>
#include <QElapsedTimer>

class AudioRecorder : public QObject
{
    Q_OBJECT

public:
    explicit AudioRecorder(QObject *parent = nullptr);
    ~AudioRecorder();

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE bool isRecording() const;

Q_SIGNALS:
    void recordingStarted();
    void recordingFinished(QString tempFilePath);
    void errorOccurred(QString message);

private:
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioDevice = nullptr;
    QTemporaryFile *m_tempFile = nullptr;
    bool m_isRecording = false;
    QByteArray m_audioData;

    // Voice Activity Detection (VAD)
    QTimer *m_silenceTimer = nullptr;
    QElapsedTimer m_lastVoiceTime;
    bool m_hasDetectedVoice = false;

    static constexpr int SILENCE_THRESHOLD = 500;      // RMS threshold for silence detection
    static constexpr int SILENCE_DURATION_MS = 1500;   // Stop after 1.5s of silence
    static constexpr int MIN_RECORDING_MS = 500;       // Minimum recording duration

    void cleanup();
    void processAudioData(const QByteArray &data);
    qint16 calculateRMS(const QByteArray &data);
};

#endif // AUDIORECORDER_H
