#ifndef DICTATIONCONTROLLER_H
#define DICTATIONCONTROLLER_H

#include <QObject>

class AudioRecorder;
class WhisperTranscriber;
class YdotoolWriter;

class DictationController : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)

public:
  explicit DictationController(AudioRecorder *recorder,
                               WhisperTranscriber *transcriber,
                               YdotoolWriter *writer,
                               QObject *parent = nullptr);

  bool isActive() const;

public Q_SLOTS:
  void start();
  void stop();

Q_SIGNALS:
  void activeChanged(bool active);
  void errorOccurred(QString message);

private:
  void handleError(const QString &message);
  void setActive(bool active);
  void notify(const QString &eventId, const QString &message);

  AudioRecorder *m_recorder;
  WhisperTranscriber *m_transcriber;
  YdotoolWriter *m_writer;
  bool m_active = false;
};

#endif // DICTATIONCONTROLLER_H
