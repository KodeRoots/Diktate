#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QObject>
#include <QWindow>

class KStatusNotifierItem;

class SystemTray : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit SystemTray(QObject *parent = nullptr);
    ~SystemTray();

    bool isVisible() const;
    void setVisible(bool visible);

    Q_INVOKABLE void setWindow(QWindow *window);

Q_SIGNALS:
    void visibleChanged();
    void quitRequested();

private:
    void onActivateRequested(bool active);
    void onQuitRequested();

    KStatusNotifierItem *m_trayIcon = nullptr;
    QWindow *m_window = nullptr;
    bool m_visible = true;
};

#endif // SYSTEMTRAY_H
