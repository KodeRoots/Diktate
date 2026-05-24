#include "systemtray.h"
#include <KStatusNotifierItem>
#include <KLocalizedString>
#include <QApplication>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
{
    m_trayIcon = new KStatusNotifierItem(QStringLiteral("org.koderoots.diktate"), this);
    m_trayIcon->setIconByName(QStringLiteral("org.koderoots.diktate"));
    m_trayIcon->setTitle(i18n("Diktate"));
    m_trayIcon->setCategory(KStatusNotifierItem::ApplicationStatus);
    m_trayIcon->setStatus(KStatusNotifierItem::Active);

    m_trayIcon->contextMenu()->addAction(i18n("Show Diktate"));
    m_trayIcon->contextMenu()->addSeparator();
    QAction *quitAction = m_trayIcon->contextMenu()->addAction(i18n("Quit"));

    connect(m_trayIcon, &KStatusNotifierItem::activateRequested,
            this, &SystemTray::onActivateRequested);

    connect(quitAction, &QAction::triggered, this, &SystemTray::onQuitRequested);

    connect(m_trayIcon->contextMenu()->actions().constFirst(), &QAction::triggered,
            this, [this]() { setVisible(true); });
}

SystemTray::~SystemTray()
{
}

bool SystemTray::isVisible() const
{
    return m_visible;
}

void SystemTray::setVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }

    m_visible = visible;

    if (m_window) {
        if (visible) {
            m_window->show();
            m_window->raise();
            m_window->requestActivate();
        } else {
            m_window->hide();
        }
    }

    Q_EMIT visibleChanged();
}

void SystemTray::setWindow(QWindow *window)
{
    m_window = window;
}

void SystemTray::onActivateRequested(bool active)
{
    Q_UNUSED(active)
    setVisible(!m_visible);
}

void SystemTray::onQuitRequested()
{
    Q_EMIT quitRequested();
}
