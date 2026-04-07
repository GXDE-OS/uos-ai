#pragma once

#include "chatmemory.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QTimer>

class Session;

namespace uos_ai {
namespace chatbot {

class ChannelManager;

/**
 * @brief ChatBotRequestProcessor - AI 请求生命周期管理
 *
 * 职责：
 * 1. 根据 payload 构建多轮上下文，调用 Session::requestMcpAgent 发起请求
 * 2. 处理流式/非流式响应（onChatText*、onChatError、onChatContext）
 * 3. 超时保护：60s 无响应则取消并通知用户
 * 4. 收集完整 agent context，写回 ChatMemory
 * 5. 账号选择（UOS_FREE 优先，否则 fallback）
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

    void setSession(Session *session);
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
    void onChatTextReceived(const QString &id, const QString &chatText);
    void onChatTextChunkReceived(const QString &id, const QString &deltaText);
    void onChatContextReceived(const QString &id, const QJsonArray &context);
    void onChatError(const QString &id, int code, const QString &errorString);
    void onLlmAccountListChanged();
    void onChannelError(const QString &platform, const QString &error);

private:
    QString ensureSession(const QString &memKey);
    QString resolveAccountId() const;
    void    onRequestDone(const QString &reqId, const QString &targetKey);
    QString toolUseToReadable(const QJsonObject &msg);

    /** 发送最终回复：流式平台 update+finalize，非流式平台 sendMessage */
    void sendFinalReply(const QString &platform, const QString &to,
                        const QString &convType, const QString &streamHandle,
                        const QString &msg);

    struct ReplyTarget {
        QString    platform;
        QString    to;
        QString    conversationType;
        QString    convKey;          // sessionKey，用于写回 memory
        QString    userText;         // 原始用户消息
        QString    streamHandle;     // 非空表示已启动流式卡片
        QString    accumulatedText;  // 流式模式下累积的完整文本
        QJsonArray agentContext;     // chatContextReceived 缓存的工具调用链
        bool       inToolCall   = false;
        bool       pendingFlush = false;
        QTimer    *timeoutTimer = nullptr;
    };

    QMap<QString, ReplyTarget>  m_pendingRequests;  // reqId → target
    QHash<QString, QString>     m_activeConvKeys;   // memKey → sessionKey

    Session        *m_session  = nullptr;
    ChannelManager *m_channels = nullptr;
    ChatMemory     *m_memory;

    QString m_uosFreeAccountId;
    QString m_fallbackAccountId;

    static constexpr int REQUEST_TIMEOUT_MS = 600000;
};

} // namespace chatbot
} // namespace uos_ai
