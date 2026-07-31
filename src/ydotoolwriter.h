#ifndef YDOTOOLWRITER_H
#define YDOTOOLWRITER_H

#include <QList>
#include <QObject>
#include <QProcess>
#include <QStringList>

class YdotoolWriter : public QObject {
  Q_OBJECT

public:
  explicit YdotoolWriter(QObject *parent = nullptr);

  bool isAvailable() const;

  void typeText(const QString &text);

Q_SIGNALS:
  void errorOccurred(QString message);
  void finished();

private:
  void typeNextChunk();
  QStringList argsForChunk(const QStringList &chunk) const;
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void onProcessError(QProcess::ProcessError error);

  QString m_executable;
  QProcess *m_process = nullptr;
  QList<QStringList> m_chunks;
  bool m_writing = false;
  bool m_useFlatpakSpawn = false;
  bool m_useDistroboxExec = false;
};

#endif // YDOTOOLWRITER_H
