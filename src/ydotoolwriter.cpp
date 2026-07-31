#include "ydotoolwriter.h"
#include <KLocalizedString>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace {
// KEY_ENTER (28) press/release, from linux/input-event-codes.h
constexpr char ENTER_PRESS[] = "28:1";
constexpr char ENTER_RELEASE[] = "28:0";
} // namespace

YdotoolWriter::YdotoolWriter(QObject *parent) : QObject(parent) {
  // Inside a Flatpak sandbox, talk to the system ydotool via flatpak-spawn
  const bool inFlatpak = !qEnvironmentVariable("FLATPAK_ID").isEmpty();
  if (inFlatpak) {
    m_executable =
        QStandardPaths::findExecutable(QStringLiteral("flatpak-spawn"));
    m_useFlatpakSpawn = !m_executable.isEmpty();
  }

  if (m_executable.isEmpty()) {
    m_executable = QStandardPaths::findExecutable(QStringLiteral("ydotool"));
  }
  for (const QString &path : {QStringLiteral("/usr/bin/ydotool"),
                              QStringLiteral("/usr/local/bin/ydotool")}) {
    if (m_executable.isEmpty() && QFile::exists(path)) {
      m_executable = path;
      break;
    }
  }

  // Fall back to distrobox-host-exec when running inside a distrobox container
  if (m_executable.isEmpty()) {
    m_executable =
        QStandardPaths::findExecutable(QStringLiteral("distrobox-host-exec"));
    m_useDistroboxExec = !m_executable.isEmpty();
  }

  if (m_executable.isEmpty()) {
    qWarning() << "YdotoolWriter: ydotool not found on the system";
  }

  m_process = new QProcess(this);

  // Let the ydotool client find the daemon socket (common dev setup).
  // Not needed for flatpak-spawn: the host process inherits the host env.
  if (!m_useFlatpakSpawn && qEnvironmentVariableIsEmpty("YDOTOOL_SOCKET")) {
    const QString homeSocket =
        QDir::homePath() + QStringLiteral("/.ydotool_socket");
    if (QFile::exists(homeSocket)) {
      QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
      env.insert(QStringLiteral("YDOTOOL_SOCKET"), homeSocket);
      m_process->setProcessEnvironment(env);
    }
  }

  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          &YdotoolWriter::onProcessFinished);
  connect(m_process, &QProcess::errorOccurred, this,
          &YdotoolWriter::onProcessError);
}

bool YdotoolWriter::isAvailable() const { return !m_executable.isEmpty(); }

void YdotoolWriter::typeText(const QString &text) {
  if (!isAvailable()) {
    Q_EMIT errorOccurred(i18n("ydotool is not installed. Install it and make "
                              "sure the ydotoold daemon is running."));
    return;
  }

  if (m_writing) {
    Q_EMIT errorOccurred(i18n("Previous text is still being typed"));
    return;
  }

  m_chunks.clear();
  const QStringList lines = text.split(QLatin1Char('\n'));
  for (int i = 0; i < lines.size(); ++i) {
    if (i > 0) {
      m_chunks.append(QStringList{QLatin1String("key"),
                                  QLatin1String(ENTER_PRESS),
                                  QLatin1String(ENTER_RELEASE)});
    }
    m_chunks.append(QStringList{QLatin1String("type"), lines.at(i)});
  }

  m_writing = true;
  typeNextChunk();
}

void YdotoolWriter::typeNextChunk() {
  if (m_chunks.isEmpty()) {
    m_writing = false;
    Q_EMIT finished();
    return;
  }

  m_process->start(m_executable, argsForChunk(m_chunks.takeFirst()));
}

QStringList YdotoolWriter::argsForChunk(const QStringList &chunk) const {
  if (m_useFlatpakSpawn) {
    QStringList args{QLatin1String("--host"), QLatin1String("ydotool")};
    args.append(chunk);
    return args;
  }

  if (m_useDistroboxExec) {
    QStringList args = chunk;
    args.prepend(QStringLiteral("ydotool"));
    return args;
  }

  return chunk;
}

void YdotoolWriter::onProcessFinished(int exitCode,
                                      QProcess::ExitStatus exitStatus) {
  if (exitStatus != QProcess::NormalExit || exitCode != 0) {
    m_writing = false;
    m_chunks.clear();
    Q_EMIT errorOccurred(i18n("ydotool failed (exit code %1). Make sure the "
                              "ydotoold daemon is running.",
                              exitCode));
    return;
  }

  typeNextChunk();
}

void YdotoolWriter::onProcessError(QProcess::ProcessError error) {
  Q_UNUSED(error)
  m_writing = false;
  m_chunks.clear();
  Q_EMIT errorOccurred(
      i18n("Failed to run ydotool: %1", m_process->errorString()));
}
