#include "chatbotrequestprocessor.h"
#include "chatbotpayload.h"
#include "channelmanager.h"
#include "chatbotagent.h"
#include "session.h"
#include "networkdefs.h"
#include "global_define.h"
#include "serverdefs.h"
#include "tooluse.h"
#include "agentfactory.h"
#include "mcpserver.h"
#include "dconfigmanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(logProcessor, "uos-ai.chatbot.processor")

using namespace uos_ai::chatbot;

ChatBotRequestProcessor::ChatBotRequestProcessor(ChatMemory *memory, QObject *parent)
    : QObject(parent)
    , m_memory(memory)
{
}

void ChatBotRequestProcessor::setSession(Session *session)
{
    m_session = session;
    connect(m_session, &Session::chatTextReceived,
            this, &ChatBotRequestProcessor::onChatTextReceived);
    connect(m_session, &Session::chatTextChunkReceived,
            this, &ChatBotRequestProcessor::onChatTextChunkReceived);
    connect(m_session, &Session::chatContextReceived,
            this, &ChatBotRequestProcessor::onChatContextReceived);
    connect(m_session, &Session::error,
            this, &ChatBotRequestProcessor::onChatError);
    connect(m_session, &Session::llmAccountLstChanged,
            this, [this](const QString &, const QString &) { onLlmAccountListChanged(); });

    onLlmAccountListChanged();
}

void ChatBotRequestProcessor::setChannelManager(ChannelManager *channels)
{
    m_channels = channels;
    connect(m_channels, &ChannelManager::channelError,
            this, &ChatBotRequestProcessor::onChannelError);
}

// ============================================================
// public
// ============================================================

void ChatBotRequestProcessor::process(const QJsonObject &payload)
{
    const ParsedPayload p = ParsedPayload::from(payload);
    const QString sessionKey = ensureSession(p.memKey());

    const QString accountId = resolveAccountId();
    if (accountId.isEmpty()) {
        const QString errMsg = tr("No model found. Please configure a model first.");
        m_channels->sendMessage(p.platform, p.replyTo, errMsg, p.convType);
        onRequestDone(QString(), p.platform + ":" + p.replyTo);
        return;
    }

    const QString ctx = QString::fromUtf8(QJsonDocument(
        m_memory->buildContext(sessionKey, p.content)
    ).toJson(QJsonDocument::Compact));

    // 获取 chatbot agent 可用的 MCP 服务器
    QStringList mcpServers;
    {
        QSharedPointer<MCPServer> mcpServer =
            AgentFactory::instance()->getMCPServer(QString(uos_ai::kDefaultAgentName));
        if (mcpServer) {
            mcpServer->scanServers();
            const QStringList enabledList =
                DConfigManager::instance()->value(MCP_GROUP, MCP_ENABLED_LIST).toStringList();
            for (const QString &s : mcpServer->serverNames()) {
                if (enabledList.contains(s))
                    mcpServers.append(s);
            }
        }
    }

    QVariantHash params;
    params.insert(QLatin1String(PREDICT_PARAM_STREAM), true);
    params.insert(QLatin1String(PREDICT_PARAM_NOSOCKET), true);
    params.insert(QLatin1String(PREDICT_PARAM_THINKCHAIN), QVariant(false));
    params.insert(QLatin1String(PREDICT_PARAM_MCPAGENT), QString(uos_ai::chatbot::kChatbotAgentName));
    params.insert(QLatin1String(PREDICT_PARAM_MCPSERVERS), QVariant(mcpServers));
    params.insert(QLatin1String(uos_ai::chatbot::kChatbotPlatformParam), p.platform);

    const QString historySummary = m_memory->summaryFor(sessionKey);
    if (!historySummary.isEmpty())
        params.insert(QLatin1String(uos_ai::chatbot::kChatbotHistorySummaryParam), historySummary);

    QString streamHandle = m_channels->beginStreamingReply(p.platform, p.replyTo, p.convType);
    auto    reply        = m_session->requestMcpAgent(accountId, ctx, params);
    QString reqId        = reply.second.value(0);

    if (reply.first != AIServer::NoError || reqId.isEmpty()) {
        sendFinalReply(p.platform, p.replyTo, p.convType, streamHandle, tr("AI request failed"));
        onRequestDone(QString(), p.platform + ":" + p.replyTo);
        return;
    }

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(REQUEST_TIMEOUT_MS);
    connect(timer, &QTimer::timeout, this, [this, reqId,
            platform = p.platform, replyTo = p.replyTo,
            convType = p.convType, streamHandle] {
        if (!m_pendingRequests.contains(reqId))
            return;

        qCWarning(logProcessor) << "Request timed out:" << reqId;
        m_session->cancelRequestTask(reqId);
        sendFinalReply(platform, replyTo, convType, streamHandle,
                       tr("Request timed out, please try again"));

        onRequestDone(reqId, platform + ":" + replyTo);
    });
    timer->start();

    ReplyTarget target;
    target.platform         = p.platform;
    target.to               = p.replyTo;
    target.conversationType = p.convType;
    target.convKey          = sessionKey;
    target.userText         = p.content;
    target.streamHandle     = streamHandle;
    target.timeoutTimer     = timer;
    m_pendingRequests.insert(reqId, target);
}

void ChatBotRequestProcessor::cancelForTarget(const QString &platform, const QString &replyTo,
                                               const QString &convType)
{
    const QString targetKey = platform + ":" + replyTo;

    QString activeReqId;
    for (auto it = m_pendingRequests.cbegin(); it != m_pendingRequests.cend(); ++it) {
        if (it.value().platform == platform && it.value().to == replyTo) {
            activeReqId = it.key();
            break;
        }
    }

    if (activeReqId.isEmpty()) {
        m_channels->sendMessage(platform, replyTo, tr("No active request to stop."), convType);
        return;
    }

    qCInfo(logProcessor) << "User requested stop for reqId:" << activeReqId;
    m_session->cancelRequestTask(activeReqId);

    const ReplyTarget &target = m_pendingRequests[activeReqId];
    sendFinalReply(target.platform, target.to, target.conversationType,
                   target.streamHandle, tr("Request cancelled."));
    onRequestDone(activeReqId, targetKey);
}

void ChatBotRequestProcessor::cancelAll()
{
    const QList<QString> pendingIds = m_pendingRequests.keys();
    for (const QString &reqId : pendingIds) {
        m_session->cancelRequestTask(reqId);
        ReplyTarget &t = m_pendingRequests[reqId];
        if (t.timeoutTimer) {
            t.timeoutTimer->stop();
            t.timeoutTimer->deleteLater();
            t.timeoutTimer = nullptr;
        }
        if (!t.streamHandle.isEmpty())
            m_channels->finalizeStreamingReply(t.platform, t.streamHandle);
    }
    m_pendingRequests.clear();
    m_activeConvKeys.clear();
}

void ChatBotRequestProcessor::newSession(const QString &memKey)
{
    const QString sessionKey = memKey + "#" + QString::number(QDateTime::currentMSecsSinceEpoch());
    m_activeConvKeys.insert(memKey, sessionKey);
}

// ============================================================
// private slots
// ============================================================

void ChatBotRequestProcessor::onChatContextReceived(const QString &id, const QJsonArray &context)
{
    // 在 onChatTextReceived 之前到达（session_p 保证 context 先 emit）
    if (!m_pendingRequests.contains(id))
        return;
    m_pendingRequests[id].agentContext = context;
}

void ChatBotRequestProcessor::onChatTextChunkReceived(const QString &id, const QString &deltaText)
{
    if (!m_pendingRequests.contains(id))
        return;

    ReplyTarget &target = m_pendingRequests[id];
    if (target.streamHandle.isEmpty())
        return;

    QString delta;
    if (deltaText.startsWith('{')) {
        QJsonObject obj = QJsonDocument::fromJson(deltaText.toUtf8()).object();
        QJsonObject msg = obj.value("message").toObject();
        int chatType    = msg.value("chatType").toInt(-1);

        if (chatType == ChatTextPlain) {
            delta = msg.value("content").toString();
            if (target.inToolCall)
                target.inToolCall = false;
        } else if (chatType == ChatToolUse) {
            int status = msg.value("status").toInt(ToolUse::Calling);
            if (status == ToolUse::Calling)
                target.inToolCall = true;
            delta = toolUseToReadable(msg);
        }
    } else {
        delta = deltaText;
    }

    if (delta.isEmpty())
        return;

    target.accumulatedText += delta;

    if (target.pendingFlush)
        return;

    target.pendingFlush = true;
    QTimer::singleShot(200, this, [this, id] {
        if (!m_pendingRequests.contains(id))
            return;
        ReplyTarget &t = m_pendingRequests[id];
        t.pendingFlush = false;
        if (!t.streamHandle.isEmpty() && !t.accumulatedText.isEmpty())
            m_channels->updateStreamingReply(t.platform, t.streamHandle, t.accumulatedText);
    });
}

void ChatBotRequestProcessor::onChatTextReceived(const QString &id, const QString &chatText)
{
    if (!m_pendingRequests.contains(id))
        return;

    ReplyTarget target = m_pendingRequests[id];  // copy，下面 onRequestDone 会 take

    QString replyText;
    if (chatText.startsWith('{')) {
        QJsonObject obj = QJsonDocument::fromJson(chatText.toUtf8()).object();
        replyText = obj.value("message").toObject().value("content").toString();
    }
    if (replyText.isEmpty())
        replyText = chatText;

    if (replyText.isEmpty()) {
        onRequestDone(id, target.platform + ":" + target.to);
        return;
    }

    // 存储完整轮次（用户消息 + agent context 链含工具调用）
    if (!target.agentContext.isEmpty()) {
        m_memory->appendTurn(target.convKey, target.userText, target.agentContext);
    } else {
        // 降级：context 信号未到达时退回纯文本存储
        m_memory->append(target.convKey, "user", target.userText);
        m_memory->append(target.convKey, "assistant", replyText);
    }

    sendFinalReply(target.platform, target.to, target.conversationType,
                   target.streamHandle, replyText);
    onRequestDone(id, target.platform + ":" + target.to);
}

void ChatBotRequestProcessor::onChatError(const QString &id, int code, const QString &errorString)
{
    if (!m_pendingRequests.contains(id))
        return;

    ReplyTarget target = m_pendingRequests[id];
    const QString errorMsg = errorString.isEmpty()
                             ? tr("AI error (code: %1)").arg(code)
                             : errorString;
    sendFinalReply(target.platform, target.to, target.conversationType,
                   target.streamHandle, errorMsg);
    onRequestDone(id, target.platform + ":" + target.to);
}

void ChatBotRequestProcessor::onLlmAccountListChanged()
{
    if (!m_session)
        return;

    const QJsonArray accounts = QJsonDocument::fromJson(
        m_session->queryLLMAccountList().toUtf8()).array();

    m_uosFreeAccountId.clear();
    for (const QJsonValue &v : accounts) {
        const QJsonObject obj = v.toObject();
        if (obj.value("model").toInt() == static_cast<int>(LLMChatModel::UOS_FREE)) {
            m_uosFreeAccountId = obj.value("id").toString();
            break;
        }
    }

    m_fallbackAccountId = m_session->currentLLMAccountId();

    qCDebug(logProcessor) << "UOS_FREE account id:" << (m_uosFreeAccountId.isEmpty() ? "(none)" : m_uosFreeAccountId)
                          << "fallback account id:" << (m_fallbackAccountId.isEmpty() ? "(none)" : m_fallbackAccountId);
}

// ============================================================
// private
// ============================================================

QString ChatBotRequestProcessor::ensureSession(const QString &memKey)
{
    auto it = m_activeConvKeys.find(memKey);
    if (it != m_activeConvKeys.end())
        return it.value();

    const QString prefix = memKey + "#";
    QString bestKey;
    qint64  bestTime = 0;
    for (const QString &k : m_memory->activeKeys()) {
        if (!k.startsWith(prefix))
            continue;
        qint64 t = m_memory->stats(k).lastActiveMs;
        if (t > bestTime) {
            bestTime = t;
            bestKey  = k;
        }
    }

    if (bestKey.isEmpty())
        bestKey = prefix + QString::number(QDateTime::currentMSecsSinceEpoch());

    m_activeConvKeys.insert(memKey, bestKey);
    return bestKey;
}

QString ChatBotRequestProcessor::resolveAccountId() const
{
    return m_uosFreeAccountId.isEmpty() ? m_fallbackAccountId : m_uosFreeAccountId;
}

void ChatBotRequestProcessor::onRequestDone(const QString &reqId, const QString &targetKey)
{
    if (!reqId.isEmpty() && m_pendingRequests.contains(reqId)) {
        ReplyTarget &t = m_pendingRequests[reqId];
        if (t.timeoutTimer) {
            t.timeoutTimer->stop();
            t.timeoutTimer->deleteLater();
            t.timeoutTimer = nullptr;
        }
        m_pendingRequests.remove(reqId);
    }

    emit requestFinished(targetKey);
}

void ChatBotRequestProcessor::onChannelError(const QString &platform, const QString &error)
{
    // 取消该平台所有进行中的请求，并尽力通知用户
    const QList<QString> pendingIds = m_pendingRequests.keys();
    for (const QString &reqId : pendingIds) {
        ReplyTarget &t = m_pendingRequests[reqId];
        if (t.platform != platform)
            continue;

        qCWarning(logProcessor) << "Cancelling request" << reqId
                                << "due to channel error on" << platform;
        m_session->cancelRequestTask(reqId);

        if (t.timeoutTimer) {
            t.timeoutTimer->stop();
            t.timeoutTimer->deleteLater();
            t.timeoutTimer = nullptr;
        }

        // 平台出错时发送消息可能失败，但尽力尝试
        sendFinalReply(t.platform, t.to, t.conversationType, t.streamHandle,
                       tr("Platform error, request cancelled: %1").arg(error));

        const QString targetKey = t.platform + ":" + t.to;
        m_pendingRequests.remove(reqId);
        emit requestFinished(targetKey);
    }
}

void ChatBotRequestProcessor::sendFinalReply(const QString &platform, const QString &to,
                                              const QString &convType,
                                              const QString &streamHandle,
                                              const QString &msg)
{
    if (!streamHandle.isEmpty()) {
        m_channels->updateStreamingReply(platform, streamHandle, msg);
        m_channels->finalizeStreamingReply(platform, streamHandle);
    } else {
        m_channels->sendMessage(platform, to, msg, convType);
    }
}

QString ChatBotRequestProcessor::toolUseToReadable(const QJsonObject &msg)
{
    const QString name   = msg.value("name").toString();
    const int     status = msg.value("status").toInt(ToolUse::Calling);

    switch (static_cast<ToolUse::Staus>(status)) {
    case ToolUse::Calling:
        return QString("\n*Calling tool: %1*\n").arg(name);
    case ToolUse::Completed:
        return QString();
    case ToolUse::Failed:
        return QString("\n*Tool %1 failed*\n").arg(name);
    default:
        return QString();
    }
}
