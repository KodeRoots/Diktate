#include "dictationcontroller.h"
#include "audiorecorder.h"
#include "whispertranscriber.h"
#include "ydotoolwriter.h"
#include <KLocalizedString>
#include <KNotification>
#include <QDebug>

DictationController::DictationController(AudioRecorder *recorder,
                                         WhisperTranscriber *transcriber,
                                         YdotoolWriter *writer, QObject *parent)
    : QObject(parent), m_recorder(recorder), m_transcriber(transcriber),
      m_writer(writer) {
  connect(m_recorder, &AudioRecorder::recordingStarted, this, [this]() {
    // Only take over recordings initiated by the dictation path (tray /
    // global shortcut). Recordings started from the app window are handled
    // entirely by the QML batch flow.
    if (!m_ownSession) {
      return;
    }
    setActive(true);
    notify(QStringLiteral("dictationListening"), i18n("Listening..."));
    m_transcriber->beginSession();
  });

  connect(m_recorder, &AudioRecorder::segmentReady, this,
          [this](const QByteArray &pcmData) {
            if (!m_active) {
              return;
            }
            m_transcriber->transcribeSegment(pcmData);
          });

  connect(m_recorder, &AudioRecorder::recordingFinished, this,
          [this](const QString &) {
            if (!m_active) {
              return;
            }
            notify(QStringLiteral("dictationTranscribing"),
                   i18n("Transcribing..."));
            m_transcriber->finishSession();
          });

  connect(m_recorder, &AudioRecorder::errorOccurred, this,
          &DictationController::handleError);

  connect(m_transcriber, &WhisperTranscriber::transcriptionComplete, this,
          [this](const QString &text) {
            if (!m_active) {
              return;
            }
            setActive(false);
            m_ownSession = false;
            const QString trimmed = text.trimmed();
            if (trimmed.isEmpty()) {
              notify(QStringLiteral("dictationError"),
                     i18n("No speech detected"));
              return;
            }
            m_writer->typeText(trimmed);
          });

  connect(m_transcriber, &WhisperTranscriber::errorOccurred, this,
          &DictationController::handleError);
  connect(m_writer, &YdotoolWriter::errorOccurred, this,
          &DictationController::handleError);

  connect(m_writer, &YdotoolWriter::finished, this, [this]() {
    notify(QStringLiteral("dictationComplete"), i18n("Text typed"));
  });
}

bool DictationController::isActive() const { return m_active; }

void DictationController::start() {
  if (m_active || m_recorder->isRecording()) {
    return;
  }

  if (!m_transcriber->isModelLoaded()) {
    handleError(i18n(
        "No Whisper model loaded. Open Diktate and download a model first."));
    return;
  }

  if (!m_writer->isAvailable()) {
    handleError(i18n("ydotool is not available. Install it and make sure the "
                     "ydotoold daemon is running."));
    return;
  }

  m_ownSession = true;
  m_recorder->startRecording();
}

void DictationController::stop() {
  if (!m_active) {
    return;
  }

  m_recorder->stopRecording();
}

void DictationController::handleError(const QString &message) {
  if (m_active) {
    setActive(false);
  }

  m_ownSession = false;
  m_transcriber->cancelSession();

  notify(QStringLiteral("dictationError"), message);
  Q_EMIT errorOccurred(message);
}

void DictationController::setActive(bool active) {
  if (m_active == active) {
    return;
  }

  m_active = active;
  Q_EMIT activeChanged(m_active);
}

void DictationController::notify(const QString &eventId,
                                 const QString &message) {
  KNotification::event(eventId, i18n("Diktate"), message,
                       QStringLiteral("org.koderoots.diktate"));
}
