#ifndef CHATBOTREQUESTHANDLER_H
#define CHATBOTREQUESTHANDLER_H

#include "chatmemory.h"

#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QQueue>
#include <QSet>

class Session;

namespace uos_ai {
namespace chatbot {

class ChannelManager;
class ChatBotRequestProcessor;
class ChatBotCommandHandler;

/**
 * @brief ChatBotRequestHandler - 消息路由与并发调度
 *
 * 职责：
 * 1. 接收 ChannelManager 转发的 IM 平台消息
 * 2. 将 / 开头的指令委托给 ChatBotCommandHandler
 * 3. 并发控制：同一目标（platform:to）同时只处理一条，其余排队
 * 4. 监听 ChatBotRequestProcessor::requestFinished，释放并发锁并驱动队列
 *
 * AI 请求生命周期由 ChatBotRequestProcessor 负责；
 * 指令处理由 ChatBotCommandHandler 负责。
 */
class ChatBotRequestHandler : public QObject
{
    Q_OBJECT

public:
    explicit ChatBotRequestHandler(QObject *parent = nullptr);

    void setSession(Session *session);
    void setChannelManager(ChannelManager *manager);

private Q_SLOTS:
    void onMessageReceived(const QJsonObject &payload);
    void onRequestFinished(const QString &targetKey);

private:
    ChannelManager          *m_channelManager = nullptr;
    ChatMemory               m_memory;
    ChatBotRequestProcessor *m_processor      = nullptr;
    ChatBotCommandHandler   *m_commandHandler = nullptr;

    QSet<QString>                      m_activeTargets;
    QMap<QString, QQueue<QJsonObject>> m_queue;
};

} // namespace chatbot
} // namespace uos_ai

#endif // CHATBOTREQUESTHANDLER_H
