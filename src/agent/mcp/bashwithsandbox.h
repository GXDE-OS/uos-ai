#ifndef BASHWITHSANDBOX_H
#define BASHWITHSANDBOX_H

#include "conversation/messagenode.h"
#include "sandbox/sandbox_types.h"

#include <QObject>
#include <QWaitCondition>
#include <QMutex>

namespace uos_ai {

class BashWithSandbox : public QObject
{
    Q_OBJECT
public:
    explicit BashWithSandbox(bool alwaysApprove = false, QObject *parent = nullptr);
    QPair<int, QString> execute(const QString &command, QString dir, QString exp, bool inSandbox);
    inline bool skipApproval() const {
        return m_alwaysApprove;
    }
signals:
    void sendRequsets(const RenderMessageList &msgs);
    void requestAbort();
public slots:
    void actionCallback(const QJsonObject &action);
    void abort();
protected:
    // 应用文件变更到真实文件系统
    bool applyFileChanges(const QList<sandbox::FsChange> &changes,
                         const QString &lowerdir,
                         const QString &upperdir);
    QPair<int, QString> runInSandobox(const QString &command, QString dir);
    QPair<int, QString> run(const QString &command, QString dir);
    bool requestFileChange(const QList<sandbox::FsChange>& changes, const QString& lowerDir, const QString& mergedDir);
private:
    static QList<sandbox::FsChange> filterChanges(const QList<sandbox::FsChange> &changes,const QString &lowerDir, bool log = false);
    QWaitCondition m_waitCallback;
    QMutex m_waitMtx;
    QMap<QString, QJsonObject> m_replay;
    bool m_alwaysApprove;
    bool m_abort = false;
};

}

#endif // BASHWITHSANDBOX_H
