#include <gio/gio.h>

#include "bashwithsandbox.h"
#include "icompents.h"
#include "osinfo.h"
#include "sandbox/sandbox.h"

#include <QThread>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

BashWithSandbox::BashWithSandbox(bool alwaysApprove, QObject *parent)
    : QObject(parent)
    , m_alwaysApprove(alwaysApprove)
{

}

QPair<int, QString> BashWithSandbox::execute(const QString &command, QString dir, QString exp, bool inSandbox)
{
    // 检查是否需要用户授权
    bool approved = !inSandbox || m_alwaysApprove;
    QString rejectMsg;

    if (!approved) {
        m_waitMtx.lock();
        qCInfo(logAgent) << "request user approve the command" << command << "dir" << dir;
        QString requestId = GlobalUtil::generateMsId();
        QVariantHash cardData = makeBashApprove(requestId, exp, command);
        RenderMessage msg;
        msg.type = CntIComps;
        msg.data = cardData;
        emit sendRequsets({msg});

        if (!m_abort)
            m_waitCallback.wait(&m_waitMtx);

        m_waitMtx.unlock();
        QJsonObject reply = m_replay.take(requestId);
        m_alwaysApprove = reply.value("always_approve").toBool(false);
        if (m_alwaysApprove)
            qCWarning(logAgent) << "user always approve the command!!!";
        approved = m_alwaysApprove || reply.value("approved").toBool(false);
        rejectMsg = reply.value("reject_msg").toString();
    } else {
        qCWarning(logAgent) << "user always approve the command" << inSandbox << m_alwaysApprove << command << dir;
    }

    QPair<int, QString> ret;
    if (!approved || m_abort) {
        ret = {GErrorType::MCPToolError, QString("User reject the command.%1")
               .arg(rejectMsg.isEmpty() ? "" : QString("\nAnd input reason: %1").arg(rejectMsg))};
        qCWarning(logAgent) << "user reject the command";
    } else {
        if (inSandbox)
            ret = runInSandobox(command, dir);
        else
            ret = run(command, dir);
    }

    return ret;
}

void BashWithSandbox::actionCallback(const QJsonObject &action)
{
    QString id = action.value("request_id").toString();
    if (id.isEmpty()) {
        return;
    }

    m_replay.insert(id, action);
    m_waitCallback.notify_all();
}

void BashWithSandbox::abort()
{
    m_abort = true;
    m_waitCallback.notify_all();
    emit requestAbort();
}

QList<sandbox::FsChange> BashWithSandbox::filterChanges(const QList<sandbox::FsChange> &changes, const QString &lowersDir, bool log)
{
    QStringList ignored;
    QList<sandbox::FsChange> filtered;
    QString trashPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Trash";
    for (const auto &change : changes) {
        QString path = lowersDir + "/" + change.path;
        if (change.isDir && change.kind == sandbox::ChangeKind::Modified) { // 忽略文件夹修改
            ignored.append(path);
        } else {
            if (!path.startsWith(trashPath)) {
                filtered.append(change);
            } else if (log) {
                ignored.append(path);
            }
        }
    }

    if (log)
        qCInfo(logAgent) << "Ignore changes" << ignored;

    return filtered;
}

bool BashWithSandbox::applyFileChanges(const QList<sandbox::FsChange> &changes, const QString &lowerdir, const QString &upperdir)
{
    if (changes.isEmpty()) {
        return true;
    }

    qCWarning(logAgent) << "Applying" << changes.size() << "file changes";

    // 按路径深度排序的辅助函数
    auto getDepth = [](const QString &path) {
        return path.count('/');
    };

    // 分组变更：删除、创建、修改
    QList<sandbox::FsChange> deletions;
    QList<sandbox::FsChange> creations;
    QList<sandbox::FsChange> modifications;

    for (const auto &change : changes) {
        switch (change.kind) {
        case sandbox::ChangeKind::Deleted:
            deletions.append(change);
            break;
        case sandbox::ChangeKind::Created:
            creations.append(change);
            break;
        case sandbox::ChangeKind::Modified:
            if (!change.isDir) // 不能应用文件夹修改
                modifications.append(change);
            break;
        }
    }

    // 删除操作：按深度从深到浅排序（先删除深层文件/目录）
    std::sort(deletions.begin(), deletions.end(),
              [&getDepth](const sandbox::FsChange &a, const sandbox::FsChange &b) {
                  return getDepth(a.path) > getDepth(b.path);
              });

    // 创建操作：按深度从浅到深排序（先创建浅层目录）
    std::sort(creations.begin(), creations.end(),
              [&getDepth](const sandbox::FsChange &a, const sandbox::FsChange &b) {
                  return getDepth(a.path) < getDepth(b.path);
              });

    bool success = true;

    // 1. 执行删除操作
    for (const auto &change : deletions) {
        QString realPath = lowerdir + "/" + change.path;

        if (change.isDir) {
            QDir dir(realPath);
            if (dir.exists()) {
                qCInfo(logAgent) << "beigin remove directory to trash:" << realPath ;
                GFile *file = g_file_new_for_path(realPath.toStdString().c_str());
                if (file) {
                    if (!g_file_trash(file, nullptr, nullptr)) {
                        qCInfo(logAgent) << "Failed to remove directory:" << realPath;
                        success = false;
                    } else {
                        qCWarning(logAgent) << "Removed directory:" << realPath ;
                    }
                    g_object_unref(file);
                }
            }
        } else {
            if (QFile::exists(realPath)) {
                qCInfo(logAgent) << "beigin remove file to trash:" << realPath ;
                GFile *file = g_file_new_for_path(realPath.toStdString().c_str());
                if (file) {
                    if (!g_file_trash(file, nullptr, nullptr)) {
                        qCInfo(logAgent) << "Failed to remove file:" << realPath;
                        success = false;
                    } else {
                        qCWarning(logAgent) << "Removed file:" << realPath ;
                    }
                    g_object_unref(file);
                }
            }
        }
    }

    // 2. 执行创建/修改操作
    for (const auto &change : creations + modifications) {
        QString upperPath = upperdir + "/" + change.path;
        QString realPath = lowerdir + "/" + change.path;

        // 确保目标目录存在
        QFileInfo realInfo(realPath);
        QString parentDir = realInfo.absolutePath();
        if (!QDir().mkpath(parentDir)) {
            qCWarning(logAgent) << "Failed to create parent directory:" << parentDir;
            success = false;
            continue;
        }

        if (change.isDir) {
            // 创建目录
            if (!QDir().mkpath(realPath)) {
                qCWarning(logAgent) << "Failed to create directory:" << realPath;
                success = false;
            } else {
                qCWarning(logAgent) << "Created directory:" << realPath;
            }
        } else {
            // 检查是否为符号链接
            QFileInfo upperInfo(upperPath);

            // 先删除目标文件（如果存在）
            if (QFile::exists(realPath)) {
                qCInfo(logAgent) << "beigin remove file to trash:" << realPath ;
                GFile *file = g_file_new_for_path(realPath.toStdString().c_str());
                if (file) {
                    if (!g_file_trash(file, nullptr, nullptr)) {
                        qCWarning(logAgent) << "Failed to remove file:" << realPath;
                        success = false;
                    } else {
                        qCWarning(logAgent) << "Removed file:" << realPath ;
                    }
                    g_object_unref(file);
                }
            }

            if (upperInfo.isSymLink()) {
                // 创建符号链接，保留相对/绝对链接类型
                char linkPath[4096] = {0};
                ssize_t size = readlink(upperPath.toUtf8().constData(), linkPath, 4095);
                QString target = QString::fromUtf8(linkPath, size);
                if (QFile::link(target, realPath)) {
                    qCWarning(logAgent) << "Created symlink:" << realPath << "->" << target;
                } else {
                    qCWarning(logAgent) << "Failed to create symlink:" << realPath << "->" << target;
                    success = false;
                }
            } else if (QFile::copy(upperPath, realPath)) {
                // 复制文件权限
                QFile upperFile(upperPath);
                QFile realFile(realPath);

                // 获取源文件权限
                QFlags<QFileDevice::Permission> perms = upperFile.permissions();
                realFile.setPermissions(perms);

                qCWarning(logAgent) << "Copied file:" << upperPath << "to" << realPath;
            } else {
                qCWarning(logAgent) << "Failed to copy file:" << upperPath << "to" << realPath;
                success = false;
            }
        }
    }

    return success;
}

QPair<int, QString> BashWithSandbox::runInSandobox(const QString &command, QString dir)
{
    // 在沙箱中执行
    GErrorType err = GErrorType::NoError;
    QString output;

    try {
        // 确定工作目录
        if (dir.isEmpty()) {
            dir = QDir::homePath();
        } else if (dir.startsWith("~")) {
            dir = QDir::homePath() + dir.mid(1);
        }

        // 创建沙箱配置
        sandbox::SandboxConfig config(QDir::homePath());
        sandbox::SandboxExecutor executor(config);

        connect(this, &BashWithSandbox::requestAbort, &executor, &sandbox::SandboxExecutor::abort, Qt::DirectConnection);
        // 准备回调函数的标志位
        bool userConfirmedChanges = false;

        if (m_abort)
            return qMakePair(GErrorType::NoError, QString("user canceled"));

        // 执行命令，传入回调函数
        auto result = executor.simulate(
            command,
            dir,
            false,
            std::nullopt,
            std::nullopt,
            1000 * 60 * 30, // 30分钟超时
            [this, &userConfirmedChanges](const QList<sandbox::FsChange>& changes, const QString& lowerDir, const QString& mergedDir) {
                if (m_abort) {
                    userConfirmedChanges = false;
                    return ;
                }
                userConfirmedChanges = requestFileChange(filterChanges(changes, lowerDir, true), lowerDir, mergedDir);
            }
        );

        if (m_abort)
            return qMakePair(GErrorType::NoError, QString("user canceled"));

        // 应用文件变更
        if (!result.changes.isEmpty()) {
            auto realChange = filterChanges(result.changes, result.lowerdir);
            if (!realChange.isEmpty()) {
                if (userConfirmedChanges) {
                    if (!applyFileChanges(realChange, result.lowerdir, result.upperdir)) {
                        qCWarning(logAgent) << "Failed to apply some file changes";
                    }
                } else {
                    output = "INFO: The command was executed inside a sandbox. File changes occurred within the sandbox, but the user rejected applying those changes to the real system.\n";
                }
            }
        }

        if (result.cache.startsWith(QDir::tempPath())) {
            QDir(result.cache).removeRecursively();
        } else {
            qCCritical(logAgent) << "the sandbox work dir is not in tmp";
        }

        // 返回命令执行结果
        if (result.exit_code != 0)
            err = GErrorType::MCPToolError;

        output = QString("%0exit code:%1\nstdout:%2\nstderr:%3\n")
                .arg(output)
                .arg(result.exit_code)
                .arg(result.stdout_output)
                .arg(result.stderr_output);
    } catch (const sandbox::SandboxUnavailableError& e) {
        qCCritical(logAgent) << "Sandbox error:" << e.what();
        return {GErrorType::MCPToolError,
                QString("Sandbox execution failed: %1\nDetails: %2").arg(e.getReason(),
                e.getDetails().value_or(""))};
    } catch (const std::exception& e) {
        qCCritical(logAgent) << "Sandbox execution error:" << e.what();
        return {GErrorType::MCPToolError,
                QString("Sandbox execution error: %1").arg(e.what())};
    }

    return {GErrorType::NoError, output};
}

QPair<int, QString> BashWithSandbox::run(const QString &command, QString dir)
{
    QProcess process;
    connect(this, &BashWithSandbox::requestAbort, &process, &QProcess::terminate, Qt::DirectConnection);
    process.setProcessEnvironment(UosInfo()->pureEnvironment());
    if (!dir.isEmpty()) {
        if (dir.startsWith("~")) {
            dir = QDir::homePath() + dir.mid(1);
        }

        process.setWorkingDirectory(dir);
    }

    if (m_abort)
        return qMakePair(GErrorType::NoError, QString("user canceled"));

    qCWarning(logAgent) << "execute command in real:" << command << "in dir:" << dir;
    process.start("bash", QStringList() << "-lc" << command);
    if (!process.waitForFinished(1000 * 60 * 30)) { // 30分钟超时
        process.kill();
        return qMakePair(1, QString("Command execution timeout"));
    }

    QThread::sleep(1);

    GErrorType err = GErrorType::NoError;
    QString output;

    if (process.exitCode() != 0)
        err = GErrorType::MCPToolError;

    output = QString("%0exit code:%1\nstdout:%2\nstderr:%3\n")
            .arg(output)
            .arg(process.exitCode())
            .arg(QString::fromLocal8Bit(process.readAllStandardOutput()))
            .arg(QString::fromLocal8Bit(process.readAllStandardError()));


    if (output.isEmpty())
        output = "Command has executed, but no ouput.";
    return qMakePair(err, output);
}

bool BashWithSandbox::requestFileChange(const QList<sandbox::FsChange> &changes, const QString &lowerDir, const QString &mergedDir)
{
    QVariantList changeList;
    for (const auto &change : changes) {
        QVariantHash item;
        item["path"] = lowerDir + "/" + change.path;
        item["kind"] = change.kind == sandbox::ChangeKind::Created ? "created"
                     : change.kind == sandbox::ChangeKind::Modified ? "modified"
                     : "deleted";
        item["is_dir"] = change.isDir;
        changeList.append(item);
    }

    qCWarning(logAgent) << "request user approve the file changes" << changeList;
    if (changeList.isEmpty())
        return false;

    m_waitMtx.lock();
    QString requestId = GlobalUtil::generateMsId();
    QVariantHash cardData = makeFileChangeApprove(requestId, changeList, changes.size());
    RenderMessage msg;
    msg.type = CntIComps;
    msg.data = cardData;
    emit sendRequsets({msg});

    if (!m_abort)
        m_waitCallback.wait(&m_waitMtx);

    m_waitMtx.unlock();
    QJsonObject reply = m_replay.take(requestId);
    return reply.value("approve").toBool(false);
}
