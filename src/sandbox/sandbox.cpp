#include "sandbox.h"
#include "osinfo.h"

#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QSet>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QLoggingCategory>

#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/xattr.h>

Q_DECLARE_LOGGING_CATEGORY(logSandbox)

using namespace uos_ai;
using namespace sandbox;

// ==================== SandboxUnavailableError ====================

SandboxUnavailableError::SandboxUnavailableError(
    const QString& reason,
    const std::optional<QString>& details)
    : std::runtime_error(details.has_value() ? (reason + ": " + details.value()).toStdString() : reason.toStdString()),
      reason(reason),
      details(details) {
}

// ==================== 工具函数 ====================

std::tuple<QString, bool, bool> stripSudoPrefix(const QString& command) {
    if (command.isEmpty()) {
        return {"", false, true};
    }

    // 跳过前导空格
    int idx = 0;
    while (idx < command.size() && command[idx].isSpace()) {
        idx++;
    }

    // 检查是否以"sudo"开头
    if (idx + 4 >= command.size() || command.mid(idx, 4) != "sudo") {
        return {command, false, true};
    }

    // 检查sudo后面是否是空格
    if (idx + 4 < command.size() && !command[idx + 4].isSpace()) {
        return {command, false, true};
    }

    idx += 4;

    // 跳过所有sudo选项
    while (idx < command.size()) {
        while (idx < command.size() && command[idx].isSpace()) {
            idx++;
        }

        if (idx >= command.size() || command[idx] != '-') {
            break;
        }

        // 跳过选项
        while (idx < command.size() && !command[idx].isSpace()) {
            idx++;
        }

        // 跳过空格
        while (idx < command.size() && command[idx].isSpace()) {
            idx++;
        }

        // 如果选项需要参数，也跳过
        if (idx < command.size() && command[idx] != '-') {
            while (idx < command.size() && !command[idx].isSpace()) {
                idx++;
            }
        }
    }

    QString stripped = command.mid(idx);
    if (stripped.isEmpty()) {
        return {"", true, false};
    }

    return {stripped, true, true};
}

// ==================== SandboxExecutor ====================

SandboxExecutor::SandboxExecutor(const SandboxConfig& config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{

}

CommandResult SandboxExecutor::runCommand(const QStringList& args,
                                          const std::optional<int> &timeout, bool canAbort) {
    CommandResult result;

    if (args.isEmpty()) {
        result.returncode = -1;
        return result;
    }

    QProcess process;
    process.setProgram(args.first());
    process.setArguments(args.mid(1));
    process.setProcessEnvironment(UosInfo()->pureEnvironment());
    if (canAbort) {
        connect(this, &SandboxExecutor::requestAbort, &process, &QProcess::terminate, Qt::DirectConnection);
        if (m_abort) {
            result.returncode = -1;
            return result;
        }
    }

    process.start();
    if (!process.waitForStarted()) {
        result.returncode = -1;
        result.stderr_output = QString("Failed to start process: %1").arg(process.errorString());
        return result;
    }

    // 设置超时（默认30秒）
    int timeoutMs = timeout.has_value() ? timeout.value() : 30000;

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished();
        result.returncode = -1;
        result.stderr_output = "Process timed out";
        return result;
    }

    result.stdout_output = QString::fromUtf8(process.readAllStandardOutput());
    result.stderr_output = QString::fromUtf8(process.readAllStandardError());
    result.returncode = process.exitCode();

    return result;
}

void SandboxExecutor::mountOverlay(const QString& lowerdir, const QString& upperdir,
                                    const QString& workdir, const QString& merged) {
    QString options = QString("lowerdir=%1,upperdir=%2,workdir=%3")
                         .arg(lowerdir, upperdir, workdir);

    QStringList args = {
        "fuse-overlayfs", "-o", options, merged
    };
    qCInfo(logSandbox) << "mount overlay:" << lowerdir  << upperdir << workdir << merged;
    auto result = runCommand(args);
    if (result.returncode != 0) {
        QString first_line = result.stderr_output.section('\n', 0, 0);

        throw SandboxUnavailableError("overlay_mount_failed",
            first_line.isEmpty() ? QString("exit_code=%1").arg(result.returncode) : first_line);
    }
}

void SandboxExecutor::umount(const QString& path) {
    qCInfo(logSandbox) << "unmount overlay:" << path;
    auto result = runCommand({"fusermount", "-u", path});

    if (result.returncode == 0) {
        return;
    }

    result = runCommand({"fusermount", "-uz", path});
    if (result.returncode == 0) {
        return;
    }

    qCWarning(logSandbox) << "[sandbox] WARNING: failed to umount" << path;
    if (!result.stderr_output.isEmpty()) {
        qCWarning(logSandbox) << result.stderr_output;
    }
}

void SandboxExecutor::prepareOverlayDirsForUser(const QString& upperdir,
                                                 const QString& workdir,
                                                 int uid, int gid) {
    if (chown(upperdir.toUtf8().constData(), uid, gid) < 0 ||
        chown(workdir.toUtf8().constData(), uid, gid) < 0) {
        throw SandboxUnavailableError("overlay_perm_failed", QString::fromUtf8(std::strerror(errno)));
    }

    if (chmod(upperdir.toUtf8().constData(), 0700) < 0 ||
        chmod(workdir.toUtf8().constData(), 0700) < 0) {
        throw SandboxUnavailableError("overlay_perm_failed", QString::fromUtf8(std::strerror(errno)));
    }
}

QList<FsChange> SandboxExecutor::collectFsChanges(
    const QString& lowerdir,
    const QString& upperdir,
    const QString& repo_root) {

    QList<FsChange> changes;
    QSet<QString> upper_files;
    QSet<QString> deleted_paths;

    QDir upperDir(upperdir);
    QDir lowerDir(lowerdir);

    // 辅助函数：记录删除
    auto record_deleted = [&](const QString& path_value, bool dir) {
        if (deleted_paths.contains(path_value)) {
            return;
        }

        // upper中文件被删除，无法判断是否为文件夹，这里使用lower来判断
        struct stat st;
        if (lstat(lowerDir.filePath(path_value).toUtf8().constData(), &st) == 0) {
            dir = S_ISDIR(st.st_mode);
        } else { // 文件不存在或无权限
            qCWarning(logSandbox) << "record deleted, lstat path failed: " << lowerDir.filePath(path_value) << QString::fromUtf8(std::strerror(errno));
            return;
        }

        deleted_paths.insert(path_value);
        changes.append(FsChange(path_value, ChangeKind::Deleted, dir));
    };

    // 辅助函数：构建元数据详细信息
    auto build_meta_detail = [&](const QString& upper_path,
                                  const QString& lower_path) -> std::optional<QMap<QString, QString>> {
        struct stat up, low;
        if (lstat(upper_path.toUtf8().constData(), &up) < 0 ||
            lstat(lower_path.toUtf8().constData(), &low) < 0) {
            return std::nullopt;
        }

        QMap<QString, QString> detail;
        if ((up.st_mode & 07777) != (low.st_mode & 07777)) {
            detail["mode"] = QString("%1->%2")
                .arg(low.st_mode & 07777, 0, 8)
                .arg(up.st_mode & 07777, 0, 8);
        }
        if (up.st_uid != low.st_uid) {
            detail["uid"] = QString("%1->%2").arg(low.st_uid).arg(up.st_uid);
        }
        if (up.st_gid != low.st_gid) {
            detail["gid"] = QString("%1->%2").arg(low.st_gid).arg(up.st_gid);
        }

        return detail.isEmpty() ? std::nullopt : std::optional(detail);
    };

    // 辅助函数：递归遍历目录
    std::function<void(const QString&)> traverseDir;
    traverseDir = [&](const QString& currentPath) {
        QDir dir(currentPath);
        if (!dir.exists()) {
            return;
        }

        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        for (const QFileInfo& entry : entries) {
            QString path = entry.absoluteFilePath();
            QString rel = QDir(upperdir).relativeFilePath(path);
            bool isDir = entry.isDir();
            // 检查opaque目录
            if (isDir) {
                // 递归遍历子目录
                traverseDir(path);
            }

            // 处理文件
            QString filename = entry.fileName();

            if (filename.size() >= 4 && filename.left(4) == ".wh.") {
                // whiteout文件
                QString target_name = filename.mid(4);
                QString target_rel = QDir(upperdir).relativeFilePath(
                    QFileInfo(path).path() + "/" + target_name);
                QString lower_path = lowerdir + "/" + target_rel;

                QString logical_rel;
                QDir repoRoot(repo_root);
                if (repoRoot.isAbsolutePath(lower_path)) {
                    logical_rel = repoRoot.relativeFilePath(lower_path);
                } else {
                    logical_rel = lower_path;
                }

                record_deleted(logical_rel, isDir);
            } else {
                // 普通文件或目录
                QString lower_path = lowerdir + "/" + rel;

                QString logical_rel;
                QDir repoRoot(repo_root);
                if (repoRoot.isAbsolutePath(lower_path)) {
                    logical_rel = repoRoot.relativeFilePath(lower_path);
                } else {
                    logical_rel = lower_path;
                }

                // 检查是否是字符设备whiteout
                struct stat st;
                if (lstat(path.toUtf8().constData(), &st) == 0) {
                    if (S_ISCHR(st.st_mode) && st.st_rdev == 0) {
                        record_deleted(logical_rel, isDir);
                        continue;
                    }
                }

                if (entry.isFile()) {
                    upper_files.insert(rel);
                }

                QFileInfo lowerFileInfo(lower_path);
                if (lowerFileInfo.exists()) {
                    changes.append(FsChange(logical_rel, ChangeKind::Modified, isDir));
                } else {
                    changes.append(FsChange(logical_rel, ChangeKind::Created, isDir));
                }
            }
        }
    };

    // 遍历upperdir
    try {
        traverseDir(upperdir);
    } catch (const std::exception& e) {
        qCWarning(logSandbox) << "[sandbox] Error collecting fs changes:" << e.what();
    }

    return changes;
}

CommandResult SandboxExecutor::runInBubblewrap(
    const QString& lower_root,
    const QString& merged_root,
    const QString& workdir,
    const QString& command,
    const std::optional<int>& run_as_uid,
    const std::optional<int>& run_as_gid,
    const std::optional<int>& timeout_s) {

    QStringList bwrap_cmd = {
        "bwrap",
        "--dev", "/dev",
        "--proc", "/proc"
    };

    // 添加只读挂载
    auto readonly_binds = m_config.readonly_binds;
    if (!readonly_binds.has_value()) {
        QDir repoRoot(m_config.repo_root);
        if (repoRoot.canonicalPath() != "/") {
            readonly_binds = QList<QPair<QString, QString>>{
                {"/", "/"},
            };
        } else {
            readonly_binds = QList<QPair<QString, QString>>{};
        }
    }

    for (const auto& bind : readonly_binds.value()) {
        bwrap_cmd.append({"--ro-bind", bind.first, bind.second});
    }

    // 添加读写挂载
    if (m_config.readwrite_binds.has_value()) {
        for (const auto& bind : m_config.readwrite_binds.value()) {
            bwrap_cmd.append({"--bind", bind.first, bind.second});
        }
    }

    bwrap_cmd.append({"--chdir", workdir});
    bwrap_cmd.append({"--bind", merged_root, lower_root});

    // 添加用户权限设置
    if (run_as_uid.has_value() || run_as_gid.has_value()) {
        if (!run_as_uid.has_value() || !run_as_gid.has_value()) {
            throw SandboxUnavailableError("bubblewrap_failed",
                "run_as_uid/run_as_gid must both be set");
        }

        bwrap_cmd.append({
            "setpriv",
            "--reuid", QString::number(run_as_uid.value()),
            "--regid", QString::number(run_as_gid.value()),
            "--clear-groups",
            "--inh-caps=-all"
        });
    }

    bwrap_cmd.append({"bash", "-lc", command});
    qCWarning(logSandbox) << "execute command in bubblewrap:" << bwrap_cmd.join(" ");

    return runCommand(bwrap_cmd, timeout_s, true);
}

SandboxResult SandboxExecutor::runInOverlaySandbox(
    const QString& command,
    const QString& repo_root,
    const QString& cwd,
    bool noCache,
    const std::optional<int>& run_as_uid,
    const std::optional<int>& run_as_gid,
    const std::optional<int>& timeout_s,
    FsChangesCallback changesCallback) {

    QDir repoRootDir(repo_root);
    QString resolved_root = repoRootDir.canonicalPath();
    QDir cwdDir(cwd);
    QString resolved_cwd = cwdDir.canonicalPath();

    QString tmp_parent = QDir::tempPath();

    // 创建临时目录
    QString tmpdir_template = tmp_parent + "/uos-ai-sandbox-XXXXXX";

    QByteArray tmpdir_name = tmpdir_template.toUtf8();
    tmpdir_name.append('\0');

    if (mkdtemp(tmpdir_name.data()) == nullptr) {
        throw SandboxUnavailableError("tmpdir_failed", QString::fromUtf8(std::strerror(errno)));
    }

    QString tmpdir = QString::fromUtf8(tmpdir_name.constData());
    QString workdir = tmpdir + "/work";
    QString merged = tmpdir + "/merged";
    QString upperdir = tmpdir + "/upper";

    QDir().mkpath(workdir);
    QDir().mkpath(merged);
    QDir().mkpath(upperdir);

    QList<QPair<QString, QString>> overlays;
    SandboxResult result;
    bool success = false;

    try {

        if (run_as_uid.has_value() && run_as_gid.has_value()) {
            prepareOverlayDirsForUser(upperdir, workdir,
                                     run_as_uid.value(), run_as_gid.value());
        }

        mountOverlay(resolved_root, upperdir, workdir, merged);
        overlays.append({resolved_root, upperdir});

        auto proc = runInBubblewrap(
            resolved_root, merged, resolved_cwd, command,
            run_as_uid, run_as_gid, timeout_s
        );

        // 收集文件系统变更
        QList<FsChange> fs_changes;
        for (const auto& overlay : overlays) {
            auto changes = collectFsChanges(overlay.first, overlay.second, resolved_root);
            fs_changes.append(changes);
        }

        result.exit_code = proc.returncode;
        result.stdout_output = proc.stdout_output;
        result.stderr_output = proc.stderr_output;
        result.changes = fs_changes;
        result.lowerdir = resolved_root;
        result.upperdir = upperdir;
        result.cache = tmpdir;
        success = true;

        // 在 umount 之前调用回调函数
        if (changesCallback && !fs_changes.isEmpty()) {
            changesCallback(fs_changes, resolved_root, merged);
        }

    } catch (...) {
        umount(merged);
        if (noCache)
            QDir(tmpdir).removeRecursively();
        throw;
    }

    umount(merged);
    if (noCache)
        QDir(tmpdir).removeRecursively();

    return result;
}

SandboxResult SandboxExecutor::simulate(
    const QString& command,
    const QString& cwd,
    bool noCache,
    const std::optional<int>& run_as_uid,
    const std::optional<int>& run_as_gid,
    const std::optional<int>& timeout_s,
        FsChangesCallback changesCallback) {

    return runInOverlaySandbox(
        command, m_config.repo_root, cwd, noCache,
        run_as_uid, run_as_gid, timeout_s, changesCallback
                );
}

void SandboxExecutor::abort()
{
    m_abort = true;
    emit requestAbort();
}
