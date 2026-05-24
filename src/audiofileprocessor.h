#ifndef AUDIOFILEPROCESSOR_H
#define AUDIOFILEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QAudioDecoder>
#include <QTemporaryFile>
#include <QBuffer>

class AudioFileProcessor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    explicit AudioFileProcessor(QObject *parent = nullptr);
    ~AudioFileProcessor();

    bool isProcessing() const;

    Q_INVOKABLE void processFile(const QUrl &fileUrl);

Q_SIGNALS:
    void processingStarted();
    void processingFinished(const QString &tempFilePath);
    void errorOccurred(const QString &message);
    void isProcessingChanged();

private:
    void onDecoderBufferReady();
    void onDecoderFinished();
    void onDecoderError(QAudioDecoder::Error error);
    void processDecodedBuffer(const QAudioBuffer &buffer);
    void finishProcessing();
    void cleanup();

    QAudioDecoder *m_decoder = nullptr;
    QTemporaryFile *m_outputFile = nullptr;
    QBuffer m_inputBuffer;
    QByteArray m_fileData;
    bool m_isProcessing = false;
    int m_decodedBuffers = 0;
};

#endif // AUDIOFILEPROCESSOR_H
