#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "basesession.h"

#include <QObject>
#include <QMap>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QTimer>
#include <QThreadPool>

namespace uos_ai {

class BaseSession;
class ConversationRecord;

/**
 * 会话管理器，负责管理多个会话实例
 */
class SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    static QSharedPointer<SessionManager> instance(const QString &app);
    static void destroyInstance(const QString &app);

    /**
     * 创建新会话
     * @param {QString} sessionId - 会话ID（可选，为空则自动生成）
     * @returns {QJsonObject} 创建结果
     */
    QJsonObject createSession(const QString &assistId, const QString &sessionId = QString());

    /**
     * 获取会话
     * @param {QString} sessionId - 会话ID
     * @returns {SessionPtr} 会话对象，不存在返回nullptr
     */
    SessionPtr getSession(const QString &sessionId) const;

    /**
     * 删除会话
     * @param {QString} sessionId - 会话ID
     * @returns {bool} 是否删除成功
     */
    bool removeSession(const QString &sessionId);

     /**
     * 运行会话
     * @param {QString} sessionId - 会话ID
     * @param {QString} model - 模型ID
     * @param {QSharedPointer<ConversationRecord>} conversation - 对话
     * @param {QVariantHash} parameters - 参数
     * @returns {QJsonObject} 运行结果
     */
    QJsonObject runSession(const QString &sessionId, const QString &model, const QSharedPointer<ConversationRecord> &conversation,
                           const QVariantHash &parameters = {});
    
    /**
     * 获取所有会话ID列表
     * @returns {QStringList} 会话ID列表
     */
    QStringList getAllSessionIds() const;

    /**
     * 获取会话数量
     * @returns {int} 会话数量
     */
    int sessionCount() const;

    /**
     * 停止会话
     * @param {QString} sessionId - 会话ID
     */
    void cancelSession(const QString &sessionId);

signals:
    void sessionEvent(int event, const QString &id, const QString &json);
protected slots:
    void onEvent(int event, const QString &id, const QString &json);
private:
    QMap<QString, SessionPtr> sessions;      // 会话映射表
    QTimer releaseTimer;
    QList<SessionPtr> delayRelease;
    QThreadPool threadPool;

    static QMap<QString, QSharedPointer<SessionManager>> instances;
    static QReadWriteLock inslock;
};

#define WebSessMgr() SessionManager::instance("uos-ai-webfront")

} // namespace uos_ai

#endif // SESSIONMANAGER_H
