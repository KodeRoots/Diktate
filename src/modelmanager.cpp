#include "modelmanager.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>
#include <KLocalizedString>

ModelManager::ModelManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    checkCurrentModelAvailability();
}

ModelManager::~ModelManager()
{
    cancelDownload();
}

ModelManager::ModelSize ModelManager::modelSize() const
{
    return m_modelSize;
}

void ModelManager::setModelSize(ModelSize size)
{
    if (m_modelSize != size) {
        m_modelSize = size;
        Q_EMIT modelSizeChanged();
        checkCurrentModelAvailability();
    }
}

ModelManager::ModelType ModelManager::modelType() const
{
    return m_modelType;
}

void ModelManager::setModelType(ModelType type)
{
    if (m_modelType != type) {
        m_modelType = type;
        Q_EMIT modelTypeChanged();
        checkCurrentModelAvailability();
    }
}

bool ModelManager::isDownloading() const
{
    return m_isDownloading;
}

qreal ModelManager::downloadProgress() const
{
    return m_downloadProgress;
}

QString ModelManager::currentModelPath() const
{
    return getModelPath(m_modelSize, m_modelType);
}

bool ModelManager::isCurrentModelAvailable() const
{
    return m_isCurrentModelAvailable;
}

bool ModelManager::isModelAvailable(ModelSize size, ModelType type) const
{
    QString path = getModelPath(size, type);
    return QFile::exists(path);
}

QString ModelManager::modelsDirectory() const
{
    return QDir::homePath() + QStringLiteral("/.local/share/diktate/");
}

QString ModelManager::getModelFileName(ModelSize size, ModelType type) const
{
    QString sizeName;
    switch (size) {
        case Tiny: sizeName = QStringLiteral("tiny"); break;
        case Base: sizeName = QStringLiteral("base"); break;
        case Small: sizeName = QStringLiteral("small"); break;
        case Medium: sizeName = QStringLiteral("medium"); break;
        case Large: sizeName = QStringLiteral("large"); break;
    }

    if (type == EnglishOnly && size != Large) {
        return QStringLiteral("ggml-%1.en.bin").arg(sizeName);
    } else {
        return QStringLiteral("ggml-%1.bin").arg(sizeName);
    }
}

QString ModelManager::getModelPath(ModelSize size, ModelType type) const
{
    return modelsDirectory() + getModelFileName(size, type);
}

QString ModelManager::getModelDisplayName(ModelSize size, ModelType type) const
{
    QString sizeName;
    switch (size) {
        case Tiny: sizeName = i18n("Tiny"); break;
        case Base: sizeName = i18n("Base"); break;
        case Small: sizeName = i18n("Small"); break;
        case Medium: sizeName = i18n("Medium"); break;
        case Large: sizeName = i18n("Large"); break;
    }

    QString typeName = (type == EnglishOnly && size != Large) ? i18n("English") : i18n("Multilingual");
    return QStringLiteral("%1 (%2)").arg(sizeName, typeName);
}

QString ModelManager::getModelUrl(ModelSize size, ModelType type) const
{
    QString fileName = getModelFileName(size, type);
    return QStringLiteral("https://huggingface.co/ggerganov/whisper.cpp/resolve/main/%1").arg(fileName);
}

void ModelManager::checkCurrentModelAvailability()
{
    bool available = isModelAvailable(m_modelSize, m_modelType);
    if (m_isCurrentModelAvailable != available) {
        m_isCurrentModelAvailable = available;
        Q_EMIT isCurrentModelAvailableChanged();
    }
    Q_EMIT currentModelPathChanged();

    if (available) {
        Q_EMIT modelChanged(currentModelPath());
    }
}

void ModelManager::downloadCurrentModel()
{
    if (m_isDownloading) {
        return;
    }

    QString url = getModelUrl(m_modelSize, m_modelType);
    QString filePath = currentModelPath();

    // Ensure directory exists
    QDir dir(modelsDirectory());
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    // Create temporary file for download
    QString tempPath = filePath + QStringLiteral(".download");
    m_downloadFile = new QFile(tempPath, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly)) {
        Q_EMIT downloadError(i18n("Cannot create file: %1").arg(tempPath));
        delete m_downloadFile;
        m_downloadFile = nullptr;
        return;
    }

    m_isDownloading = true;
    m_downloadProgress = 0.0;
    Q_EMIT isDownloadingChanged();
    Q_EMIT downloadProgressChanged();
    Q_EMIT downloadStarted();

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_currentDownload = m_networkManager->get(request);
    connect(m_currentDownload, &QNetworkReply::downloadProgress, this, &ModelManager::onDownloadProgress);
    connect(m_currentDownload, &QNetworkReply::finished, this, &ModelManager::onDownloadFinished);
    connect(m_currentDownload, &QNetworkReply::errorOccurred, this, &ModelManager::onDownloadError);
    connect(m_currentDownload, &QNetworkReply::readyRead, this, [this]() {
        if (m_downloadFile) {
            m_downloadFile->write(m_currentDownload->readAll());
        }
    });
}

void ModelManager::cancelDownload()
{
    if (m_currentDownload) {
        m_currentDownload->abort();
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
    }

    if (m_downloadFile) {
        QString tempPath = m_downloadFile->fileName();
        m_downloadFile->close();
        QFile::remove(tempPath);
        delete m_downloadFile;
        m_downloadFile = nullptr;
    }

    if (m_isDownloading) {
        m_isDownloading = false;
        m_downloadProgress = 0.0;
        Q_EMIT isDownloadingChanged();
        Q_EMIT downloadProgressChanged();
    }
}

void ModelManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        m_downloadProgress = static_cast<qreal>(bytesReceived) / static_cast<qreal>(bytesTotal) * 100.0;
        Q_EMIT downloadProgressChanged();
    }
}

void ModelManager::onDownloadFinished()
{
    if (!m_currentDownload || !m_downloadFile) {
        return;
    }

    bool success = (m_currentDownload->error() == QNetworkReply::NoError);
    QString tempPath = m_downloadFile->fileName();
    QString finalPath = currentModelPath();

    m_downloadFile->close();

    if (success) {
        // Remove existing file if any
        if (QFile::exists(finalPath)) {
            QFile::remove(finalPath);
        }
        // Rename temp file to final path
        if (QFile::rename(tempPath, finalPath)) {
            Q_EMIT downloadComplete();
            checkCurrentModelAvailability();
        } else {
            QFile::remove(tempPath);
            Q_EMIT downloadError(i18n("Failed to save model file"));
        }
    } else {
        QFile::remove(tempPath);
        // Error already handled in onDownloadError
    }

    delete m_downloadFile;
    m_downloadFile = nullptr;
    m_currentDownload->deleteLater();
    m_currentDownload = nullptr;

    m_isDownloading = false;
    m_downloadProgress = 0.0;
    Q_EMIT isDownloadingChanged();
    Q_EMIT downloadProgressChanged();
}

void ModelManager::onDownloadError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error)
    if (m_currentDownload) {
        Q_EMIT downloadError(i18n("Download failed: %1").arg(m_currentDownload->errorString()));
    }
}
