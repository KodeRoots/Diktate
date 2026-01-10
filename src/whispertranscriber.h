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

public:
    explicit WhisperTranscriber(QObject *parent = nullptr);
    ~WhisperTranscriber();

    Q_INVOKABLE void transcribe(const QString &audioPath);
    Q_INVOKABLE bool isModelLoaded() const;

Q_SIGNALS:
    void transcriptionComplete(QString text);
    void transcriptionProgress(QString status);
    void errorOccurred(QString message);

private:
    struct whisper_context *m_ctx = nullptr;
    QMutex m_mutex;
    bool m_modelLoaded = false;
    QString loadModel();
    void runTranscription(const QString &audioPath);
};

#endif // WHISPERTRANSCRIBER_H
