#include "systemtray.h"
#include <KStatusNotifierItem>
#include <KLocalizedString>
#include <QApplication>
#include <QMenu>

SystemTray::SystemTray(QObject *parent)
    : QObject(parent)
{
    m_trayIcon = new KStatusNotifierItem(QStringLiteral("org.koderoots.diktate"), this);
    m_trayIcon->setIconByName(QStringLiteral("org.koderoots.diktate"));
    m_trayIcon->setTitle(i18n("Diktate"));
    m_trayIcon->setCategory(KStatusNotifierItem::ApplicationStatus);
    m_trayIcon->setStatus(KStatusNotifierItem::Active);

    QAction *showAction = m_trayIcon->contextMenu()->addAction(i18n("Show Diktate"));
    m_trayIcon->contextMenu()->addSeparator();

    connect(showAction, &QAction::triggered, this, [this]() { setVisible(true); });

    connect(m_trayIcon, &KStatusNotifierItem::activateRequested,
            this, &SystemTray::onActivateRequested);

    connect(m_trayIcon, &KStatusNotifierItem::quitRequested,
            this, &SystemTray::quitRequested);
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
