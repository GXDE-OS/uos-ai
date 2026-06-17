#ifndef SANDBOX_TYPES_H
#define SANDBOX_TYPES_H

#include <QString>
#include <QList>
#include <QVariantHash>

#include <optional>

namespace uos_ai {
namespace sandbox {

/**
 * @brief 文件系统变更类型
 */
enum class ChangeKind {
    Created,
    Modified,
    Deleted
};

/**
 * @brief 单个文件系统变更记录
 */
struct FsChange {
    QString path;              // 文件路径
    ChangeKind kind;           // 变更类型
    bool isDir;                  // 文件夹

    FsChange(const QString& p, ChangeKind k, bool dir)
        : path(p), kind(k), isDir(dir) {}
};

/**
 * @brief 文件变化回调函数类型
 * @param changes 文件变化列表
 * @param lowerdir lowerdir路径
 * @param upperdir upperdir路径
 */
using FsChangesCallback = std::function<void(const QList<FsChange>&, const QString&, const QString&)>;

/**
 * @brief 沙箱执行结果摘要
 */
struct SandboxResult {
    int exit_code;
    QString stdout_output;
    QString stderr_output;
    QList<FsChange> changes;
    bool stdout_truncated = false;
    bool stderr_truncated = false;
    bool changes_truncated = false;

    // 用于应用文件变更的路径信息
    QString cache;               //沙箱工作目录
    QString lowerdir;           // 真实文件系统路径
    QString upperdir;           // 沙箱upper层路径

    SandboxResult() : exit_code(-1) {}
};

/**
 * @brief 沙箱安全执行结果
 */
struct SandboxSecurityResult {
    QString command;
    QString cwd;
    SandboxResult sandbox;

    SandboxSecurityResult(const QString& cmd, const QString& cwd_path,
                          const SandboxResult& result)
        : command(cmd), cwd(cwd_path), sandbox(result) {}
};

} // namespace sandbox
}

#endif // SANDBOX_TYPES_H
