#ifndef WHISPERTRANSCRIBER_H
#define WHISPERTRANSCRIBER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QStringList>
#include <QMap>
#include <thread>
#include <vector>

#include <whisper.h>

class WhisperTranscriber : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentModelPath READ currentModelPath NOTIFY modelLoaded)
    Q_PROPERTY(bool useGpu READ useGpu WRITE setUseGpu NOTIFY useGpuChanged)
    Q_PROPERTY(int gpuDevice READ gpuDevice WRITE setGpuDevice NOTIFY gpuDeviceChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList supportedLanguages READ supportedLanguages CONSTANT)

public:
    explicit WhisperTranscriber(QObject *parent = nullptr);
    ~WhisperTranscriber();

    Q_INVOKABLE void transcribe(const QString &audioPath);
    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void loadModel(const QString &modelPath);

    // Incremental dictation session: speech segments are transcribed as they
    // arrive, so most of the work is done by the time the user stops speaking.
    Q_INVOKABLE void beginSession();
    Q_INVOKABLE void transcribeSegment(const QByteArray &pcmData);
    Q_INVOKABLE void finishSession();
    Q_INVOKABLE void cancelSession();

    QString currentModelPath() const;

    bool useGpu() const;
    void setUseGpu(bool enabled);

    int gpuDevice() const;
    void setGpuDevice(int device);

    QString language() const;
    void setLanguage(const QString &language);

    QStringList supportedLanguages() const;

    // Get display name for a language code
    Q_INVOKABLE QString languageDisplayName(const QString &code) const;

Q_SIGNALS:
    void transcriptionComplete(QString text);
    void partialTranscription(QString text);
    void transcriptionProgress(QString status);
    void errorOccurred(QString message);
    void modelLoaded(QString modelPath);
    void useGpuChanged();
    void gpuDeviceChanged();
    void languageChanged();

private:
    struct whisper_context *m_ctx = nullptr;
    QMutex m_mutex;  // guards m_ctx and serializes whisper_full calls
    bool m_modelLoaded = false;
    QString m_currentModelPath;
    bool m_useGpu = true;
    int m_gpuDevice = 0;
    QString m_language = QStringLiteral("auto");  // Default to auto-detect
    void runTranscription(const QString &audioPath);
    void reloadModelIfNeeded();

    // Transcribe float samples with the loaded model. Caller must hold m_mutex.
    QString runWhisperLocked(const std::vector<float> &samples, bool noContext);

    // Incremental session worker
    void workerLoop();
    std::thread m_workerThread;
    QMutex m_queueMutex;
    QWaitCondition m_queueCond;
    QQueue<QByteArray> m_segmentQueue;
    QStringList m_sessionParts;
    bool m_stopWorker = false;
    bool m_finishing = false;
    bool m_sessionActive = false;

    static const QMap<QString, QString> s_languageNames;
};

#endif // WHISPERTRANSCRIBER_H
