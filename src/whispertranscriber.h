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

public:
    explicit WhisperTranscriber(QObject *parent = nullptr);
    ~WhisperTranscriber();

    Q_INVOKABLE void transcribe(const QString &audioPath);
    Q_INVOKABLE bool isModelLoaded() const;
    Q_INVOKABLE void loadModel(const QString &modelPath);

    QString currentModelPath() const;

Q_SIGNALS:
    void transcriptionComplete(QString text);
    void transcriptionProgress(QString status);
    void errorOccurred(QString message);
    void modelLoaded(QString modelPath);

private:
    struct whisper_context *m_ctx = nullptr;
    QMutex m_mutex;
    bool m_modelLoaded = false;
    QString m_currentModelPath;
    void runTranscription(const QString &audioPath);
};

#endif // WHISPERTRANSCRIBER_H
