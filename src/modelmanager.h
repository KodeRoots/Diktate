#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ModelManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ModelSize modelSize READ modelSize WRITE setModelSize NOTIFY modelSizeChanged)
    Q_PROPERTY(ModelLanguage modelLanguage READ modelLanguage WRITE setModelLanguage NOTIFY modelLanguageChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY isDownloadingChanged)
    Q_PROPERTY(qreal downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(QString currentModelPath READ currentModelPath NOTIFY currentModelPathChanged)
    Q_PROPERTY(bool isCurrentModelAvailable READ isCurrentModelAvailable NOTIFY isCurrentModelAvailableChanged)

public:
    enum ModelSize {
        Tiny,
        Base,
        Small,
        Medium,
        Large
    };
    Q_ENUM(ModelSize)

    enum ModelLanguage {
        English,
        Multilingual
    };
    Q_ENUM(ModelLanguage)

    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager();

    ModelSize modelSize() const;
    void setModelSize(ModelSize size);

    ModelLanguage modelLanguage() const;
    void setModelLanguage(ModelLanguage language);

    bool isDownloading() const;
    qreal downloadProgress() const;
    QString currentModelPath() const;
    bool isCurrentModelAvailable() const;

    Q_INVOKABLE bool isModelAvailable(ModelSize size, ModelLanguage language) const;
    Q_INVOKABLE QString getModelPath(ModelSize size, ModelLanguage language) const;
    Q_INVOKABLE QString getModelFileName(ModelSize size, ModelLanguage language) const;
    Q_INVOKABLE QString getModelDisplayName(ModelSize size, ModelLanguage language) const;
    Q_INVOKABLE void downloadCurrentModel();
    Q_INVOKABLE void cancelDownload();

Q_SIGNALS:
    void modelSizeChanged();
    void modelLanguageChanged();
    void isDownloadingChanged();
    void downloadProgressChanged();
    void currentModelPathChanged();
    void isCurrentModelAvailableChanged();
    void downloadStarted();
    void downloadComplete();
    void downloadError(QString message);
    void modelChanged(QString modelPath);

private Q_SLOTS:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError error);

private:
    QString modelsDirectory() const;
    QString getModelUrl(ModelSize size, ModelLanguage language) const;
    void checkCurrentModelAvailability();

    ModelSize m_modelSize = Base;
    ModelLanguage m_modelLanguage = Multilingual;
    bool m_isDownloading = false;
    qreal m_downloadProgress = 0.0;
    bool m_isCurrentModelAvailable = false;

    QNetworkAccessManager *m_networkManager = nullptr;
    QNetworkReply *m_currentDownload = nullptr;
    QFile *m_downloadFile = nullptr;
};

#endif // MODELMANAGER_H
