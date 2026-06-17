#ifndef SANDBOX_H
#define SANDBOX_H

#include "sandbox_types.h"
#include <QString>
#include <QList>
#include <QPair>
#include <QDir>
#include <optional>
#include <tuple>
#include <functional>

namespace uos_ai {

namespace sandbox {

/**
 * @brief 沙箱不可用异常
 */
class SandboxUnavailableError : public std::runtime_error {
public:
    SandboxUnavailableError(const QString& reason,
                            const std::optional<QString>& details = std::nullopt);

    QString getReason() const { return reason; }
    std::optional<QString> getDetails() const { return details; }

private:
    QString reason;
    std::optional<QString> details;
};

/**
 * @brief 沙箱执行配置
 */
struct SandboxConfig {
    QString repo_root;                                           // 工程根路径
    bool enable_overlay = true;                                  // 是否启用overlayfs
    std::optional<QList<QPair<QString, QString>>> readonly_binds;   // 只读挂载
    std::optional<QList<QPair<QString, QString>>> readwrite_binds;  // 读写挂载

    explicit SandboxConfig(const QString& root) : repo_root(root) {}
};

/**
 * @brief 命令执行结果
 */
struct CommandResult {
    int returncode;
    QString stdout_output;
    QString stderr_output;

    CommandResult() : returncode(-1) {}
};

/**
 * @brief 沙箱执行器
 */
class SandboxExecutor : public QObject
{
    Q_OBJECT
public:
    explicit SandboxExecutor(const SandboxConfig& config, QObject *parent = nullptr);

    /**
     * @brief 在沙箱中执行命令并返回结果及文件变更列表
     * @param changesCallback 文件变化回调函数，在收集完文件变化后、umount之前调用
     */
    SandboxResult simulate(
        const QString& command,
        const QString& cwd,
        bool noCache = true,
        const std::optional<int>& run_as_uid = std::nullopt,
        const std::optional<int>& run_as_gid = std::nullopt,
        const std::optional<int>& timeout_s = std::nullopt,
        FsChangesCallback changesCallback = nullptr
    );
Q_SIGNALS:
    void requestAbort();
public Q_SLOTS:
    void abort();
private:
    SandboxConfig m_config;

    /**
     * @brief 挂载overlayfs
     */
    void mountOverlay(const QString& lowerdir, const QString& upperdir,
                      const QString& workdir, const QString& merged);

    /**
     * @brief 卸载文件系统
     */
    void umount(const QString& path);

    /**
     * @brief 执行命令
     */
    CommandResult runCommand(const QStringList& args,
                            const std::optional<int>& timeout = std::nullopt, bool canAbort = false);

    /**
     * @brief 在bubblewrap中执行命令
     */
    CommandResult runInBubblewrap(
        const QString& lower_root,
        const QString& merged_root,
        const QString& workdir,
        const QString& command,
        const std::optional<int>& run_as_uid,
        const std::optional<int>& run_as_gid,
        const std::optional<int>& timeout_s
    );

    /**
     * @brief 在overlay沙箱中执行
     * @param changesCallback 文件变化回调函数，在收集完文件变化后、umount之前调用
     */
    SandboxResult runInOverlaySandbox(
        const QString& command,
        const QString& repo_root,
        const QString& cwd,
        bool noCache = true,
        const std::optional<int>& run_as_uid = std::nullopt,
        const std::optional<int>& run_as_gid = std::nullopt,
        const std::optional<int>& timeout_s = std::nullopt,
        FsChangesCallback changesCallback = nullptr
    );

    /**
     * @brief 收集文件系统变更
     */
    QList<FsChange> collectFsChanges(
        const QString& lowerdir,
        const QString& upperdir,
        const QString& repo_root
    );

    /**
     * @brief 准备overlay目录用户权限
     */
    void prepareOverlayDirsForUser(const QString& upperdir,
                                    const QString& workdir,
                                    int uid, int gid);
private:
    bool m_abort = false;
};

/**
 * @brief 剥离sudo前缀
 */
std::tuple<QString, bool, bool> stripSudoPrefix(const QString& command);

} // namespace sandbox
}

#endif // SANDBOX_H
