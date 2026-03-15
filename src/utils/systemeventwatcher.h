#ifndef SYSTEMEVENTWATCHER_H
#define SYSTEMEVENTWATCHER_H

#include <QObject>
#include <QTimer>
#include <QDBusUnixFileDescriptor>

/**
 * @brief The SystemEventWatcher class
 * 用于监听系统级事件，如关机、重启、锁屏、应用退出等
 * 使用 systemd Inhibitor 机制延迟关机，确保前端有时间保存数据
 */
class SystemEventWatcher : public QObject
{
    Q_OBJECT
public:
    static SystemEventWatcher &instance();

    // 事件类型
    static constexpr const char *EVENT_SHUTDOWN = "shutdown";
    static constexpr const char *EVENT_RESTART = "restart";
    static constexpr const char *EVENT_LOCK = "lock";
    static constexpr const char *EVENT_QUIT = "quit";

    /**
     * @brief 前端确认已保存数据
     * 前端在保存完成后必须调用此方法，否则会等待超时
     */
    Q_INVOKABLE void confirmDataSaved();

signals:
    /**
     * @brief sigSystemEvent 系统事件信号
     * @param eventType 事件类型：shutdown/restart/lock/quit
     */
    void sigSystemEvent(const QString &eventType);

private:
    explicit SystemEventWatcher(QObject *parent = nullptr);
    ~SystemEventWatcher();

    void setupApplicationEvents();
    void setupScreenSaverDBus();

    /**
     * @brief 获取 systemd Inhibitor 锁
     * @return 是否成功获取锁
     */
    bool acquireInhibitorLock();

    /**
     * @brief 释放 Inhibitor 锁
     */
    void releaseInhibitorLock();

    /**
     * @brief 等待前端保存数据
     * @param eventType 事件类型
     */
    void waitForFrontendSave(const QString &eventType);

private slots:
    void onPrepareForShutdown(bool active);
    void onPrepareForSuspend(bool active);
    void onSessionManagerPropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void onSaveTimeout();

private:
    static SystemEventWatcher *s_instance;

    // Inhibitor 文件描述符
    QDBusUnixFileDescriptor m_inhibitorFd;
    bool m_hasInhibitor { false };

    // 前端保存状态
    bool m_frontendConfirmed { false };
    QTimer *m_saveTimer { nullptr };
    QString m_pendingEventType;

    // 超时时间（毫秒）
    static constexpr int SAVE_TIMEOUT_MS = 3000;   // 3秒超时
};

#endif   // SYSTEMEVENTWATCHER_H
