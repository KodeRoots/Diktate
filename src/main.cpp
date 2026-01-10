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

int main(int argc, char *argv[])
{
    KIconTheme::initTheme();
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("diktate");
    // QApplication::setOrganizationName(QStringLiteral("KDE"));
    // QApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QApplication::setApplicationName(QStringLiteral("Diktate"));
    QApplication::setDesktopFileName(QStringLiteral("io.github.denysmb.diktate"));

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

    engine.loadFromModule("io.github.denysmb.diktate", "Main");

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
