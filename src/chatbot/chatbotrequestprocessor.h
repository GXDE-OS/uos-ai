#pragma once

#include "chatmemory.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QTimer>

namespace uos_ai {

class SessionManager;
class ConversationRecord;

namespace chatbot {

class ChannelManager;

/**
 * @brief ChatBotRequestProcessor - AI 请求生命周期管理（基于 3.0 SessionManager）
 *
 * 职责：
 * 1. 根据 payload 构建临时 ConversationRecord，从 ChatMemory 恢复历史为 MessageNode
 * 2. 通过 SessionManager::createSession + runSession 发起请求
 * 3. 监听 sessionEvent：SeMessage 流式累积文本，SeFinished 发最终回复，SeError 报错
 * 4. 超时保护：REQUEST_TIMEOUT_MS 无响应则 cancelSession 并通知用户
 * 5. 账号选择：优先 UOS_FREE，否则取第一个文本能力模型
 * 6. 维护 memKey → sessionKey 映射（/new 通过 newSession 写入）
 *
 * 请求完成（正常/错误/超时/取消）时 emit requestFinished(targetKey)，
 * 由 ChatBotRequestHandler 据此释放并发锁并驱动队列。
 */
class ChatBotRequestProcessor : public QObject
{
    Q_OBJECT

public:
    explicit ChatBotRequestProcessor(ChatMemory *memory, QObject *parent = nullptr);

    void setChannelManager(ChannelManager *channels);

    /** 发起一次 AI 请求 */
    void process(const QJsonObject &payload);

    /**
     * @brief 取消指定 target 的进行中请求（供 /stop 使用）
     *
     * 若无进行中请求则向用户发送提示；否则取消、发送"已取消"消息并 emit requestFinished。
     */
    void cancelForTarget(const QString &platform, const QString &replyTo,
                         const QString &convType);

    /**
     * @brief 取消所有进行中请求，清空内部状态（供 /clear 使用）
     *
     * 不 emit requestFinished；调用方负责清理 queue / activeTargets。
     */
    void cancelAll();

    /** 为 memKey 创建新会话 key（供 /new 使用） */
    void newSession(const QString &memKey);

Q_SIGNALS:
    void requestFinished(const QString &targetKey);

private Q_SLOTS:
    void onSessionEvent(int event, const QString &id, const QString &json);
    void onChannelError(const QString &platform, const QString &error);

private:
    QString ensureSession(const QString &memKey);
    QString resolveModelId() const;
    void    onRequestDone(const QString &sessionId, const QString &targetKey);

    /** 发送最终回复：流式平台 update+finalize，非流式平台 sendMessage */
    void sendFinalReply(const QString &platform, const QString &to,
                        const QString &convType, const QString &streamHandle,
                        const QString &msg);

    /** 基于 ChatMemory OAI 历史 + 当前 userText 构建 ConversationRecord */
    QSharedPointer<ConversationRecord> buildConversation(const QString &convKey,
                                                         const QString &userText) const;

    struct ReplyTarget {
        QString platform;
        QString to;
        QString conversationType;
        QString convKey;          // ChatMemory key
        QString userText;         // 原始用户消息
        QString streamHandle;     // 非空表示已启动流式卡片
        QString contentSource;
        QString accumulatedText;  // 流式模式下累积的完整文本
        QJsonObject meta;
        bool    inToolCall   = false;
        bool    pendingFlush = false;
        QTimer *timeoutTimer = nullptr;
    };

    QMap<QString, ReplyTarget>  m_pendingRequests;  // sessionId → target
    QHash<QString, QString>     m_activeConvKeys;   // memKey → sessionKey

    QSharedPointer<SessionManager> m_sessionMgr;
    ChannelManager                *m_channels = nullptr;
    ChatMemory                    *m_memory;

    static constexpr int REQUEST_TIMEOUT_MS = 600000;
};

} // namespace chatbot
} // namespace uos_ai
