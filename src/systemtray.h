#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QObject>
#include <QWindow>

class KStatusNotifierItem;
class QAction;

class SystemTray : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray();

    bool isVisible() const;
    void setVisible(bool visible);

    bool isDictationActive() const;
    void setDictationActive(bool active);

    Q_INVOKABLE void setWindow(QWindow *window);

Q_SIGNALS:
    void visibleChanged();
    void quitRequested();
    void dictationToggleRequested(bool active);

private:
    void onActivateRequested(bool active);

    KStatusNotifierItem *m_trayIcon = nullptr;
    QWindow *m_window = nullptr;
    QAction *m_dictationAction = nullptr;
    bool m_visible = true;
    bool m_dictationActive = false;
};

#endif // SYSTEMTRAY_H
