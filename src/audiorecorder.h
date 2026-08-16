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
#include <memory>
#include <algorithm>

// Forward declaration
class VAD;

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
    void segmentReady(QByteArray pcmData);
    void errorOccurred(QString message);

private:
    QAudioSource *m_audioSource = nullptr;
    QIODevice *m_audioDevice = nullptr;
    QTemporaryFile *m_tempFile = nullptr;
    bool m_isRecording = false;
    QByteArray m_audioData;

    // Voice Activity Detection (VAD)
    std::unique_ptr<VAD> m_vad;
    QTimer *m_silenceTimer = nullptr;
    QElapsedTimer m_lastVoiceTime;
    bool m_hasDetectedVoice = false;

    // Sentence segmentation: audio is split into speech segments at natural
    // pauses so each sentence can be transcribed while recording continues.
    // Frames are 10ms (160 samples = 320 bytes at 16kHz int16 mono).
    static constexpr int FRAME_BYTES = 320;
    static constexpr int PRE_ROLL_BYTES = 30 * FRAME_BYTES;    // 300ms kept before speech onset
    static constexpr int TAIL_PADDING_BYTES = 20 * FRAME_BYTES; // 200ms kept after speech end
    static constexpr int MAX_SEGMENT_BYTES = 1000 * FRAME_BYTES; // 10s safety cap per segment
    static constexpr int MIN_SEGMENT_BYTES = 30 * FRAME_BYTES;   // discard noise < 300ms

    QByteArray m_pendingBytes;   // raw bytes not yet frame-aligned
    QByteArray m_preRoll;        // ring of recent frames before speech onset
    QByteArray m_segment;        // current speech segment being accumulated
    bool m_inSegment = false;

    // Fallback RMS threshold (used if VAD fails to initialize)
    static constexpr int SILENCE_THRESHOLD_RMS = 500;    // RMS threshold for silence detection
    static constexpr int SILENCE_DURATION_MS = 1500;     // Stop after 1.5s of silence
    static constexpr int MIN_RECORDING_MS = 500;         // Minimum recording duration

    void cleanup();
    void processAudioData(const QByteArray &data);
    void processFrame(const QByteArray &frame);
    void emitSegment(bool trimTrailingSilence);
    void resetSegmentation();

    // Fallback RMS-based detection (used if VAD not available)
    qint16 calculateRMS(const QByteArray &data);
};

#endif // AUDIORECORDER_H
