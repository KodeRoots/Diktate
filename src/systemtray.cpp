#include "systemtray.h"
#include <KStatusNotifierItem>
#include <KLocalizedString>
#include <QAction>
#include <QApplication>
#include <QIcon>
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
    showAction->setIcon(QIcon::fromTheme(QStringLiteral("org.koderoots.diktate")));
    m_trayIcon->contextMenu()->addSeparator();

    m_dictationAction = m_trayIcon->contextMenu()->addAction(i18n("Transcribe Microphone"));
    m_dictationAction->setCheckable(true);
    m_dictationAction->setIcon(QIcon::fromTheme(QStringLiteral("audio-input-microphone")));
    connect(m_dictationAction, &QAction::toggled, this, [this](bool checked) {
        Q_EMIT dictationToggleRequested(checked);
    });

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

bool SystemTray::isDictationActive() const
{
    return m_dictationActive;
}

void SystemTray::setDictationActive(bool active)
{
    if (m_dictationActive == active) {
        return;
    }

    m_dictationActive = active;

    m_dictationAction->setChecked(active);
    m_dictationAction->setText(active ? i18n("Stop Dictation") : i18n("Transcribe Microphone"));

    if (active) {
        m_trayIcon->setIconByName(QStringLiteral("audio-input-microphone"));
        m_trayIcon->setTitle(i18n("Diktate — Listening"));
    } else {
        m_trayIcon->setIconByName(QStringLiteral("org.koderoots.diktate"));
        m_trayIcon->setTitle(i18n("Diktate"));
    }
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
