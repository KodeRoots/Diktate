#include "audiofileprocessor.h"
#include <QAudioFormat>
#include <QFile>
#include <QDebug>
#include <KLocalizedString>
#include <cstring>

AudioFileProcessor::AudioFileProcessor(QObject *parent)
    : QObject(parent)
{
}

AudioFileProcessor::~AudioFileProcessor()
{
    cleanup();
}

bool AudioFileProcessor::isProcessing() const
{
    return m_isProcessing;
}

void AudioFileProcessor::processFile(const QUrl &fileUrl)
{
    if (m_isProcessing) {
        return;
    }

    QString localPath = fileUrl.toLocalFile();
    if (localPath.isEmpty()) {
        Q_EMIT errorOccurred(i18n("Invalid file path"));
        return;
    }

    if (!QFile::exists(localPath)) {
        Q_EMIT errorOccurred(i18n("File not found: %1", localPath));
        return;
    }

    m_isProcessing = true;
    m_decodedBuffers = 0;
    Q_EMIT isProcessingChanged();
    Q_EMIT processingStarted();

    m_outputFile = new QTemporaryFile(this);
    if (!m_outputFile->open()) {
        Q_EMIT errorOccurred(i18n("Failed to create temporary file"));
        finishProcessing();
        return;
    }

    m_decoder = new QAudioDecoder(this);
    m_decoder->setSource(QUrl::fromLocalFile(localPath));

    QAudioFormat targetFormat;
    targetFormat.setSampleRate(16000);
    targetFormat.setChannelCount(1);
    targetFormat.setSampleFormat(QAudioFormat::Int16);
    m_decoder->setAudioFormat(targetFormat);

    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioFileProcessor::onDecoderBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &AudioFileProcessor::onDecoderFinished);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error),
            this, &AudioFileProcessor::onDecoderError);

    m_decoder->start();
}

void AudioFileProcessor::onDecoderBufferReady()
{
    QAudioBuffer buffer = m_decoder->read();
    if (buffer.isValid()) {
        processDecodedBuffer(buffer);
        m_decodedBuffers++;
    }
}

void AudioFileProcessor::processDecodedBuffer(const QAudioBuffer &buffer)
{
    if (!m_outputFile || !m_outputFile->isOpen()) {
        return;
    }

    const QAudioFormat format = buffer.format();

    if (format.sampleFormat() == QAudioFormat::Int16 && format.channelCount() == 1) {
        const qint16 *data = buffer.constData<qint16>();
        int sampleCount = buffer.sampleCount();
        m_outputFile->write(reinterpret_cast<const char *>(data), sampleCount * sizeof(qint16));
    } else if (format.sampleFormat() == QAudioFormat::Float && format.channelCount() == 1) {
        const float *data = buffer.constData<float>();
        int sampleCount = buffer.sampleCount();
        QByteArray pcmData(sampleCount * sizeof(qint16), Qt::Uninitialized);
        qint16 *out = reinterpret_cast<qint16 *>(pcmData.data());
        for (int i = 0; i < sampleCount; ++i) {
            float sample = std::clamp(data[i], -1.0f, 1.0f);
            out[i] = static_cast<qint16>(sample * 32767.0f);
        }
        m_outputFile->write(pcmData);
    } else if (format.channelCount() > 1) {
        int sampleCount = buffer.sampleCount() / format.channelCount();
        int frames = buffer.frameCount();

        if (format.sampleFormat() == QAudioFormat::Int16) {
            const qint16 *data = buffer.constData<qint16>();
            QByteArray pcmData(frames * sizeof(qint16), Qt::Uninitialized);
            qint16 *out = reinterpret_cast<qint16 *>(pcmData.data());
            for (int i = 0; i < frames; ++i) {
                qint64 sum = 0;
                for (int ch = 0; ch < format.channelCount(); ++ch) {
                    sum += data[i * format.channelCount() + ch];
                }
                out[i] = static_cast<qint16>(sum / format.channelCount());
            }
            m_outputFile->write(pcmData);
        } else if (format.sampleFormat() == QAudioFormat::Float) {
            const float *data = buffer.constData<float>();
            QByteArray pcmData(frames * sizeof(qint16), Qt::Uninitialized);
            qint16 *out = reinterpret_cast<qint16 *>(pcmData.data());
            for (int i = 0; i < frames; ++i) {
                float sum = 0.0f;
                for (int ch = 0; ch < format.channelCount(); ++ch) {
                    sum += data[i * format.channelCount() + ch];
                }
                float mono = sum / static_cast<float>(format.channelCount());
                mono = std::clamp(mono, -1.0f, 1.0f);
                out[i] = static_cast<qint16>(mono * 32767.0f);
            }
            m_outputFile->write(pcmData);
        }
    }
}

void AudioFileProcessor::onDecoderFinished()
{
    if (m_decodedBuffers == 0) {
        Q_EMIT errorOccurred(i18n("No audio data could be decoded from the file"));
        finishProcessing();
        return;
    }

    if (m_outputFile) {
        m_outputFile->close();
        QString tempPath = m_outputFile->fileName();
        m_outputFile->setAutoRemove(false);
        delete m_outputFile;
        m_outputFile = nullptr;

        finishProcessing();
        Q_EMIT processingFinished(tempPath);
    } else {
        finishProcessing();
    }
}

void AudioFileProcessor::onDecoderError(QAudioDecoder::Error error)
{
    Q_UNUSED(error)
    QString msg = m_decoder ? m_decoder->errorString() : i18n("Unknown decoding error");
    Q_EMIT errorOccurred(i18n("Failed to decode audio file: %1", msg));
    finishProcessing();
}

void AudioFileProcessor::finishProcessing()
{
    m_isProcessing = false;
    Q_EMIT isProcessingChanged();
    cleanup();
}

void AudioFileProcessor::cleanup()
{
    if (m_decoder) {
        m_decoder->stop();
        delete m_decoder;
        m_decoder = nullptr;
    }

    if (m_outputFile) {
        m_outputFile->close();
        delete m_outputFile;
        m_outputFile = nullptr;
    }

    m_decodedBuffers = 0;
}
