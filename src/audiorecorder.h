#ifndef AUDIORECORDER_H
#define AUDIORECORDER_H

#include <QObject>
#include <QAudioSource>
#include <QAudioDevice>
#include <QBuffer>
#include <QFile>
#include <QTemporaryFile>

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

    void cleanup();
};

#endif // AUDIORECORDER_H
