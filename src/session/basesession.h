#ifndef BASESESSION_H
#define BASESESSION_H

#include "global_define.h"
#include "assistant/abstractassistant.h"

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantHash>

namespace uos_ai {

/**
 * 会话基类，管理AI对话会话
 */
class BaseSession : public QObject
{
    Q_OBJECT

public:
    explicit BaseSession(const QString &sessionId, QObject *parent = nullptr);
    virtual ~BaseSession();

    virtual SessionState state() const;

    /**
     * 获取会话ID
     * @returns {QString} 会话唯一标识
     */
    QString id() const;

    /**
     * 设置助手
     * @param {QString} assistID - 助手ID
     */
    virtual void setAssistant(const AssistantPtr &assist);

    /**
     * 获取助手
     * @returns {AssistantPtr} 助手对象指针
     */
    virtual AssistantPtr assistant() const;

    /**
     * 取消会话
     */
    virtual void cancel();

    /**
     * 运行会话
     * @param {QVariantHash} parameters - 参数
     * @returns {QJsonObject} 会话响应结果
     */
    virtual QJsonObject run(const QVariantHash &parameters);

signals:
    void sessionEvent(int event, const QString &id, const QString &json);
protected Q_SLOTS:
    virtual void receiveMessage(const QString &message);
    virtual void onFinished(const QVariantHash &result);
    virtual void onStarted();
    virtual void onErrorFinished(const QVariantHash &error);
protected:
    QString m_id;
    AssistantPtr m_assistant;
    SessionState m_state = SsIdle;
};

using SessionPtr = QSharedPointer<BaseSession> ;

} // namespace uos_ai

#endif // BASESESSION_H
