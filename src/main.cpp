#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QUrl>
#include <QQuickStyle>
#include <KLocalizedContext>
#include <KLocalizedString>
#include <KIconTheme>

#include "audiorecorder.h"
#include "whispertranscriber.h"
#include "modelmanager.h"
#include "audiofileprocessor.h"
#include "systemtray.h"
#include "cpuinfo.h"
#include "gpuinfo.h"

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("diktate");
    // QApplication::setOrganizationName(QStringLiteral("KDE"));
    // QApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QApplication::setApplicationName(QStringLiteral("Diktate"));
    QApplication::setDesktopFileName(QStringLiteral("org.koderoots.diktate"));

    // Log CPU and GPU features for debugging whisper.cpp performance
    CpuInfo::logInfo();
    GpuInfo::logInfo();

    QApplication::setStyle(QStringLiteral("breeze"));
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextObject(new KLocalizedContext(&engine));

    // Expose backend classes to QML
    AudioRecorder audioRecorder;
    engine.rootContext()->setContextProperty(QStringLiteral("audioRecorder"), &audioRecorder);

    WhisperTranscriber transcriber;
    engine.rootContext()->setContextProperty(QStringLiteral("transcriber"), &transcriber);

    ModelManager modelManager;
    engine.rootContext()->setContextProperty(QStringLiteral("modelManager"), &modelManager);

    AudioFileProcessor audioFileProcessor;
    engine.rootContext()->setContextProperty(QStringLiteral("audioFileProcessor"), &audioFileProcessor);

    GpuInfo gpuInfo;
    engine.rootContext()->setContextProperty(QStringLiteral("gpuInfo"), &gpuInfo);

    SystemTray systemTray;
    engine.rootContext()->setContextProperty(QStringLiteral("systemTray"), &systemTray);

    // Connect ModelManager signals to WhisperTranscriber
    QObject::connect(&modelManager, &ModelManager::modelChanged,
                     &transcriber, &WhisperTranscriber::loadModel);

    // Connect AudioFileProcessor to WhisperTranscriber
    QObject::connect(&audioFileProcessor, &AudioFileProcessor::processingFinished,
                     &transcriber, &WhisperTranscriber::transcribe);

    // Connect SystemTray quit to application quit
    QObject::connect(&systemTray, &SystemTray::quitRequested,
                     &app, &QApplication::quit);

    // Auto-load model if available on startup
    if (modelManager.isCurrentModelAvailable()) {
        transcriber.loadModel(modelManager.currentModelPath());
    }

    engine.loadFromModule("org.koderoots.diktate", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    systemTray.setWindow(qobject_cast<QWindow *>(engine.rootObjects().constFirst()));

    return app.exec();
}
