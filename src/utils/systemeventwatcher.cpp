#include "systemeventwatcher.h"

#include <QGuiApplication>
#include <QSessionManager>
#include <QLoggingCategory>

#include <QtDBus>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(logSystemEvent, "uos-ai.systemevent")

SystemEventWatcher *SystemEventWatcher::s_instance = nullptr;

SystemEventWatcher &SystemEventWatcher::instance()
{
    if (!s_instance) {
        s_instance = new SystemEventWatcher();
    }
    return *s_instance;
}

SystemEventWatcher::SystemEventWatcher(QObject *parent)
    : QObject(parent), m_saveTimer(new QTimer(this))
{
    qCDebug(logSystemEvent) << "Initializing SystemEventWatcher";

    m_saveTimer->setSingleShot(true);
    connect(m_saveTimer, &QTimer::timeout, this, &SystemEventWatcher::onSaveTimeout);

    setupApplicationEvents();
    setupScreenSaverDBus();
}

SystemEventWatcher::~SystemEventWatcher()
{
    releaseInhibitorLock();
}

void SystemEventWatcher::setupApplicationEvents()
{
    // 监听应用退出事件
    connect(qApp, &QGuiApplication::aboutToQuit, this, []() {
        qCDebug(logSystemEvent) << "Application about to quit";
        Q_EMIT instance().sigSystemEvent(EVENT_QUIT);
    });
}

void SystemEventWatcher::setupScreenSaverDBus()
{
    // 方法1: 监听 logind 的 PrepareForShutdown 信号（关机/重启）
    QDBusConnection::systemBus().connect(
            "org.freedesktop.login1",
            "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager",
            "PrepareForShutdown",
            this,
            SLOT(onPrepareForShutdown(bool)));

    // 方法2: 监听 logind 的 PrepareForSuspend 信号（休眠）
    QDBusConnection::systemBus().connect(
            "org.freedesktop.login1",
            "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager",
            "PrepareForSuspend",
            this,
            SLOT(onPrepareForSuspend(bool)));

    // 方法3: 对于 Deepin DDE，监听 Locked 属性的变化
    // 使用 org.freedesktop.DBus.Properties 的 PropertiesChanged 信号
    QDBusConnection::sessionBus().connect(
            "com.deepin.SessionManager",
            "/com/deepin/SessionManager",
            "org.freedesktop.DBus.Properties",
            "PropertiesChanged",
            this,
            SLOT(onSessionManagerPropertiesChanged(QString, QVariantMap, QStringList)));
}

void SystemEventWatcher::onSessionManagerPropertiesChanged(const QString &interfaceName,
                                                           const QVariantMap &changedProperties,
                                                           const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)

    qCDebug(logSystemEvent) << "SessionManager properties changed:" << interfaceName << changedProperties;

    if (interfaceName == "com.deepin.SessionManager" && changedProperties.contains("Locked")) {
        bool locked = changedProperties.value("Locked").toBool();
        qCDebug(logSystemEvent) << "Deepin SessionManager Locked changed:" << locked;
        if (locked) {
            Q_EMIT sigSystemEvent(EVENT_LOCK);
        }
    }
}

void SystemEventWatcher::onPrepareForShutdown(bool active)
{
    if (active) {
        qCDebug(logSystemEvent) << "PrepareForShutdown: true - system is shutting down";

        // 获取 Inhibitor 锁延迟关机
        if (acquireInhibitorLock()) {
            qCDebug(logSystemEvent) << "Acquired inhibitor lock, delaying shutdown for data save";
        }

        // 等待前端保存数据
        waitForFrontendSave(EVENT_SHUTDOWN);
    } else {
        // 关机取消，释放锁
        releaseInhibitorLock();
    }
}

void SystemEventWatcher::onPrepareForSuspend(bool active)
{
    if (active) {
        qCDebug(logSystemEvent) << "PrepareForSuspend: true - system is suspending";

        // 休眠也可能需要保存数据
        if (acquireInhibitorLock()) {
            qCDebug(logSystemEvent) << "Acquired inhibitor lock, delaying suspend for data save";
        }

        waitForFrontendSave(EVENT_LOCK);
    } else {
        releaseInhibitorLock();
    }
}

bool SystemEventWatcher::acquireInhibitorLock()
{
    if (m_hasInhibitor) {
        qCDebug(logSystemEvent) << "Inhibitor already acquired";
        return true;
    }

    // 调用 systemd logind 的 Inhibit 方法
    QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.freedesktop.login1",
            "/org/freedesktop/login1",
            "org.freedesktop.login1.Manager",
            "Inhibit");

    // 参数：what, who, why, mode
    // what: shutdown|sleep|idle|handle-power-key|handle-suspend-key|handle-hibernate-key|handle-lid-switch
    // mode: block(阻塞直到释放) | delay(延迟一段时间)
    msg << QString("shutdown:sleep")   // 阻塞关机和休眠
        << QString("UOS AI Assistant")
        << QString("Saving user data before shutdown")
        << QString("delay");   // 延迟模式，默认5秒超时

    QDBusMessage reply = QDBusConnection::systemBus().call(msg);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logSystemEvent) << "Failed to acquire inhibitor lock:" << reply.errorMessage();
        return false;
    }

    if (reply.arguments().isEmpty()) {
        qCWarning(logSystemEvent) << "Inhibitor reply is empty";
        return false;
    }

    // 获取文件描述符
    m_inhibitorFd = qvariant_cast<QDBusUnixFileDescriptor>(reply.arguments().at(0));

    if (!m_inhibitorFd.isValid()) {
        qCWarning(logSystemEvent) << "Invalid inhibitor file descriptor";
        return false;
    }

    m_hasInhibitor = true;
    qCDebug(logSystemEvent) << "Successfully acquired inhibitor lock, fd:" << m_inhibitorFd.fileDescriptor();
    return true;
}

void SystemEventWatcher::releaseInhibitorLock()
{
    if (!m_hasInhibitor) {
        return;
    }

    qCDebug(logSystemEvent) << "Releasing inhibitor lock";

    // 关闭文件描述符即可释放锁
    // QDBusUnixFileDescriptor 析构时会自动关闭
    m_inhibitorFd = QDBusUnixFileDescriptor();
    m_hasInhibitor = false;

    qCDebug(logSystemEvent) << "Inhibitor lock released";
}

void SystemEventWatcher::waitForFrontendSave(const QString &eventType)
{
    m_frontendConfirmed = false;
    m_pendingEventType = eventType;

    qCDebug(logSystemEvent) << "Sending system event to frontend:" << eventType
                            << "- waiting for confirmation (timeout:" << SAVE_TIMEOUT_MS << "ms)";

    // 发送信号给前端
    Q_EMIT sigSystemEvent(eventType);

    // 启动超时定时器
    m_saveTimer->start(SAVE_TIMEOUT_MS);
}

void SystemEventWatcher::confirmDataSaved()
{
    qCDebug(logSystemEvent) << "Frontend confirmed data saved";

    m_frontendConfirmed = true;
    m_saveTimer->stop();

    // 释放 Inhibitor 锁，允许系统继续关机
    releaseInhibitorLock();

    qCDebug(logSystemEvent) << "System can proceed with" << m_pendingEventType;
}

void SystemEventWatcher::onSaveTimeout()
{
    if (m_frontendConfirmed) {
        return;
    }

    qCWarning(logSystemEvent) << "Frontend save timeout after" << SAVE_TIMEOUT_MS
                              << "ms - releasing inhibitor lock";

    // 超时后也要释放锁，避免阻塞系统关机
    releaseInhibitorLock();

    qCWarning(logSystemEvent) << "System will proceed with" << m_pendingEventType
                              << "even if data save is incomplete";
}
