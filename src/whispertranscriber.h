#ifndef WHISPERTRANSCRIBER_H
#define WHISPERTRANSCRIBER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>

#include <whisper.h>

class WhisperTranscriber : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentModelPath READ currentModelPath NOTIFY modelLoaded)
    Q_PROPERTY(bool useGpu READ useGpu WRITE setUseGpu NOTIFY useGpuChanged)
    Q_PROPERTY(int gpuDevice READ gpuDevice WRITE setGpuDevice NOTIFY gpuDeviceChanged)

public:
    explicit WhisperTranscriber(QObject *parent = nullptr);
    ~WhisperTranscriber();

    Q_INVOKABLE void transcribe(const QString &audioPath);
    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void loadModel(const QString &modelPath);

    QString currentModelPath() const;

    bool useGpu() const;
    void setUseGpu(bool enabled);

    int gpuDevice() const;
    void setGpuDevice(int device);

Q_SIGNALS:
    void transcriptionComplete(QString text);
    void transcriptionProgress(QString status);
    void errorOccurred(QString message);
    void modelLoaded(QString modelPath);
    void useGpuChanged();
    void gpuDeviceChanged();

private:
    struct whisper_context *m_ctx = nullptr;
    QMutex m_mutex;
    bool m_modelLoaded = false;
    QString m_currentModelPath;
    bool m_useGpu = true;
    int m_gpuDevice = 0;
    void runTranscription(const QString &audioPath);
    void reloadModelIfNeeded();
};

#endif // WHISPERTRANSCRIBER_H
