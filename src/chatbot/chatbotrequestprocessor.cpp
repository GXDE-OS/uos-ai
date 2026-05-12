#include "chatbotrequestprocessor.h"
#include "chatbotpayload.h"
#include "channelmanager.h"
#include "chatbotagent.h"
#include "chatbotassistant.h"

#include "global_key_define.h"
#include "chatbot_key_define.h"
#include "session/sessionmanager.h"
#include "conversation/conversationrecord.h"
#include "conversation/messagenode.h"
#include "model/modelvendor.h"
#include "model/modelinfo.h"
#include "model/modeltool.h"
#include "agent/mcpserver.h"
#include "agent/mcp/defaultagent.h"
#include "dconfigmanager.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(logChatBot)

using namespace uos_ai;
using namespace uos_ai::chatbot;

ChatBotRequestProcessor::ChatBotRequestProcessor(ChatMemory *memory, QObject *parent)
    : QObject(parent)
    , m_sessionMgr(SessionManager::instance(QStringLiteral("chatbot")))
    , m_memory(memory)
{
    connect(m_sessionMgr.get(), &SessionManager::sessionEvent,
            this, &ChatBotRequestProcessor::onSessionEvent);
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

    const QString modelId = resolveModelId();
    if (modelId.isEmpty()) {
        const QString errMsg = tr("No model found. Please configure a model first.");
        m_channels->sendMessage(p.platform, p.replyTo, errMsg, p.convType);
        onRequestDone(QString(), p.platform + ":" + p.replyTo);
        return;
    }

    auto conversation = buildConversation(sessionKey, p.content);
    if (conversation.isNull()) {
        sendFinalReply(p.platform, p.replyTo, p.convType, QString(), tr("AI request failed"));
        onRequestDone(QString(), p.platform + ":" + p.replyTo);
        return;
    }

    // 获取 chatbot 可用的 MCP 服务器：与 DefaultAgent 共用 MCP 服务池
    QStringList mcpServers;
    {
        QSharedPointer<MCPServer> mcpServer = DefaultAgent().mcpServer();
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
    params.insert(STR_KEY_MCP_SERVERS, QVariant(mcpServers));
    params.insert(QLatin1String(uos_ai::chatbot::kChatbotPlatformParam), p.platform);

    const QString historySummary = m_memory->summaryFor(sessionKey);
    if (!historySummary.isEmpty())
        params.insert(QLatin1String(uos_ai::chatbot::kChatbotHistorySummaryParam), historySummary);

    const QString sessionId = GlobalUtil::generateMsId();

    auto createRet = m_sessionMgr->createSession(STR_KEY_UOS_AI_CHATBOT, sessionId);
    if (createRet.value(STR_KEY_ID).toString() != sessionId) {
        qCWarning(logChatBot) << "createSession failed:" << createRet.value(STR_KEY_MESSAGE).toString();
        sendFinalReply(p.platform, p.replyTo, p.convType, QString(), tr("AI request failed"));
        onRequestDone(QString(), p.platform + ":" + p.replyTo);
        return;
    }

    QString streamHandle = m_channels->beginStreamingReply(p.platform, p.replyTo, p.convType);

    auto runRet = m_sessionMgr->runSession(sessionId, modelId, conversation, params);
    if (runRet.contains(STR_KEY_ERROR)) {
        qCWarning(logChatBot) << "runSession failed:" << runRet.value(STR_KEY_MESSAGE).toString();
        sendFinalReply(p.platform, p.replyTo, p.convType, streamHandle, tr("AI request failed"));
        onRequestDone(QString(), p.platform + ":" + p.replyTo);
        return;
    }

    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(REQUEST_TIMEOUT_MS);
    connect(timer, &QTimer::timeout, this, [this, sessionId,
            platform = p.platform, replyTo = p.replyTo,
            convType = p.convType, streamHandle] {
        if (!m_pendingRequests.contains(sessionId))
            return;

        qCWarning(logChatBot) << "Request timed out:" << sessionId;
        m_sessionMgr->cancelSession(sessionId);
        sendFinalReply(platform, replyTo, convType, streamHandle,
                       tr("Request timed out, please try again"));

        onRequestDone(sessionId, platform + ":" + replyTo);
    });
    timer->start();

    ReplyTarget target;
    target.platform         = p.platform;
    target.to               = p.replyTo;
    target.conversationType = p.convType;
    target.convKey          = sessionKey;
    target.userText         = p.content;
    target.streamHandle     = streamHandle;
    target.contentSource    = p.contentSource;
    target.meta             = p.meta;
    target.timeoutTimer     = timer;
    m_pendingRequests.insert(sessionId, target);
}

void ChatBotRequestProcessor::cancelForTarget(const QString &platform, const QString &replyTo,
                                               const QString &convType)
{
    const QString targetKey = platform + ":" + replyTo;

    QString activeSessionId;
    for (auto it = m_pendingRequests.cbegin(); it != m_pendingRequests.cend(); ++it) {
        if (it.value().platform == platform && it.value().to == replyTo) {
            activeSessionId = it.key();
            break;
        }
    }

    if (activeSessionId.isEmpty()) {
        m_channels->sendMessage(platform, replyTo, tr("No active request to stop."), convType);
        return;
    }

    qCInfo(logChatBot) << "User requested stop for sessionId:" << activeSessionId;
    m_sessionMgr->cancelSession(activeSessionId);

    const ReplyTarget &target = m_pendingRequests[activeSessionId];
    sendFinalReply(target.platform, target.to, target.conversationType,
                   target.streamHandle, tr("Request cancelled."));
    onRequestDone(activeSessionId, targetKey);
}

void ChatBotRequestProcessor::cancelAll()
{
    const QList<QString> pendingIds = m_pendingRequests.keys();
    for (const QString &sessionId : pendingIds) {
        m_sessionMgr->cancelSession(sessionId);
        ReplyTarget &t = m_pendingRequests[sessionId];
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
// private helpers
// ============================================================

// 工具名美化：去掉 server 前缀，下划线转空格
// 例: "uos-mcp-server.get_installed_apps" → "get installed apps"
static QString prettifyToolName(const QString &fullName)
{
    QString name = fullName.section(QLatin1Char('.'), -1);
    name.replace(QLatin1Char('_'), QLatin1Char(' '));
    return name.trimmed();
}

// 将 LlmAgent::processRequest 返回的 QList<ModelMessage> context 转换为 OAI 格式，
// 供 ChatMemory::appendTurn() 存储完整工具调用链
static QJsonArray modelMessagesToOAI(const QList<ModelMessage> &context)
{
    QJsonArray oai;
    for (const ModelMessage &msg : context) {
        QJsonObject out;
        out[STR_KEY_ROLE] = msg.role;

        if (msg.role == QLatin1String(STR_KEY_TOOL)) {
            for (const MetaMessage &m : msg.content) {
                if (m.type == CntTool) {
                    const QVariantHash data = m.data.toHash();
                    out[STR_KEY_TOOL_CALL_ID] = data.value(STR_KEY_TOOL_CALL_ID).toString();
                    out[STR_KEY_CONTENT]      = data.value(STR_KEY_CONTENT).toString();
                    break;
                }
            }
        } else if (msg.role == QLatin1String(STR_KEY_ASSISTANT)) {
            QJsonArray toolCalls;
            QString    textContent;
            for (const MetaMessage &m : msg.content) {
                if (m.type == CntTool) {
                    for (const ModelToolCall &call : m.data.value<ModelToolCallList>()) {
                        QJsonObject fn;
                        fn[STR_KEY_NAME]      = call.name;
                        fn[STR_KEY_ARGUMENTS] = QString::fromUtf8(
                            QJsonDocument(QJsonObject::fromVariantHash(call.arguments))
                                .toJson(QJsonDocument::Compact));
                        QJsonObject tc;
                        tc[STR_KEY_ID]       = call.id;
                        tc[STR_KEY_TYPE]     = STR_KEY_FUNCTION;
                        tc[STR_KEY_FUNCTION] = fn;
                        toolCalls.append(tc);
                    }
                } else if (m.type == CntText) {
                    textContent = m.data.toString();
                }
            }
            if (!toolCalls.isEmpty()) {
                out[STR_KEY_TOOL_CALLS] = toolCalls;
                out[STR_KEY_CONTENT]    = textContent.isEmpty()
                    ? QJsonValue(QJsonValue::Null) : QJsonValue(textContent);
            } else {
                out[STR_KEY_CONTENT] = textContent;
            }
        } else {
            continue;
        }

        if (out.size() > 1)
            oai.append(out);
    }
    return oai;
}

// ============================================================
// private slots
// ============================================================

void ChatBotRequestProcessor::onSessionEvent(int event, const QString &id, const QString &json)
{
    if (!m_pendingRequests.contains(id))
        return;

    switch (event) {
    case SessionEvent::SeStarted:
        // nothing to do
        return;

    case SessionEvent::SeMessage: {
        ReplyTarget &target = m_pendingRequests[id];
        if (target.streamHandle.isEmpty() && target.conversationType.isEmpty()) {
            // 无平台信息，忽略
            return;
        }

        const QJsonObject render = QJsonDocument::fromJson(json.toUtf8()).object();
        const QString typeStr    = render.value(STR_KEY_TYPE).toString();
        const ContentType type   = GlobalUtil::contentTypeFromString(typeStr);

        QString delta;
        if (type == CntText) {
            delta = render.value(STR_KEY_DATA).toObject().value(STR_KEY_CONTENT).toString();
            if (target.inToolCall) {
                // 工具 marker 后的首段文本：剥掉前导空白，与 marker 之间保持单空行
                int i = 0;
                while (i < delta.size() && delta.at(i).isSpace())
                    ++i;
                delta = delta.mid(i);
                target.inToolCall = false;
            }
        } else if (type == CntTool) {
            const QJsonObject data = render.value(STR_KEY_DATA).toObject();
            const QString name     = data.value(STR_KEY_NAME).toString();
            const int status       = data.value(STR_KEY_STATUS).toInt(NormalStatus::NsRunning);

            // 非流式平台：不展示中间工具调用过程，只保留最后一轮文本
            // 每次新工具调用开始时清空已累积的中间文本，最终留下的是最后一次工具调用之后的回复
            if (target.streamHandle.isEmpty()) {
                if (status == NormalStatus::NsRunning)
                    target.accumulatedText.clear();
                return;
            }

            QString line;
            if (status == NormalStatus::NsRunning)
                line = QStringLiteral("🔧 ") + tr("Calling tool: %1").arg(prettifyToolName(name));
            else if (status == NormalStatus::NsFailed)
                line = QStringLiteral("⚠️ ") + tr("Tool call failed: %1").arg(prettifyToolName(name));
            // NsCompleted: 不向用户展示工具输出

            if (!line.isEmpty()) {
                // 接缝去重：剥掉累积文本末尾空白，再用固定空行衔接
                while (!target.accumulatedText.isEmpty()
                       && target.accumulatedText.back().isSpace())
                    target.accumulatedText.chop(1);

                delta = (target.accumulatedText.isEmpty()
                             ? QString()
                             : QStringLiteral("\n\n"))
                        + line + QStringLiteral("\n\n");
                target.inToolCall = true;
            }
        }

        if (delta.isEmpty())
            return;

        target.accumulatedText += delta;

        if (target.streamHandle.isEmpty() || target.pendingFlush)
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
        return;
    }

    case SessionEvent::SeError: {
        ReplyTarget target = m_pendingRequests[id];
        const QJsonObject err = QJsonDocument::fromJson(json.toUtf8()).object();
        const QString errMsg  = err.value(STR_KEY_MESSAGE).toString();
        const int code        = err.value(STR_KEY_ERROR).toInt();
        const QString display = errMsg.isEmpty() ? tr("AI error (code: %1)").arg(code) : errMsg;

        sendFinalReply(target.platform, target.to, target.conversationType,
                       target.streamHandle, display);
        // SeError 后通常还会收到 SeFinished，由 SeFinished 统一清理
        return;
    }

    case SessionEvent::SeFinished: {
        ReplyTarget target = m_pendingRequests[id];
        const QString replyText = target.accumulatedText;

        // 保存对话历史：从 session → conversation → messageAt 取完整 context（含工具链）
        // 注：SessionManager 在本 emit 返回后才 removeSession，此处 getSession 仍有效
        const QString nodeId  = QJsonDocument::fromJson(json.toUtf8()).object()
                                    .value(STR_KEY_ID).toString();
        bool saved = false;
        if (!nodeId.isEmpty()) {
            auto session = m_sessionMgr->getSession(id);
            if (session && session->assistant() && session->assistant()->conversation()) {
                auto node = session->assistant()->conversation()->messageAt(nodeId);
                if (node) {
                    const QList<ModelMessage> ctx = node->getMessage();
                    if (!ctx.isEmpty()) {
                        m_memory->appendTurn(target.convKey, target.userText,
                                             modelMessagesToOAI(ctx));
                        saved = true;
                    }
                }
            }
        }
        if (!saved && !replyText.isEmpty()) {
            m_memory->append(target.convKey, STR_KEY_USER, target.userText);
            m_memory->append(target.convKey, STR_KEY_ASSISTANT, replyText);
        }

        // 发送最终回复
        if (!replyText.isEmpty()) {
            sendFinalReply(target.platform, target.to, target.conversationType,
                           target.streamHandle, replyText);
        } else if (!target.streamHandle.isEmpty()) {
            // 无文本但已开流式卡片：收尾避免卡片悬挂
            m_channels->finalizeStreamingReply(target.platform, target.streamHandle);
        }

        onRequestDone(id, target.platform + ":" + target.to);
        return;
    }

    default:
        return;
    }
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

QString ChatBotRequestProcessor::resolveModelId() const
{
    QVariantHash condition;
    condition.insert(STR_KEY_ARCH, ModelArch::MaLanguage);
    QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText))};
    condition.insert(STR_KEY_ABILITY, ablity);

    const QList<ModelAccountPtr> models = ModelVendor::instance()->queryModels(condition);
    if (models.isEmpty())
        return QString();

    // 优先 UOS_FREE
    for (const auto &m : models) {
        if (!m.constData())
            continue;
        if (ModelVendor::isUosProvider(m))
            return m->id;
    }

    // fallback：首个可用
    return models.first().constData() ? models.first()->id : QString();
}

void ChatBotRequestProcessor::onRequestDone(const QString &sessionId, const QString &targetKey)
{
    if (!sessionId.isEmpty() && m_pendingRequests.contains(sessionId)) {
        ReplyTarget &t = m_pendingRequests[sessionId];
        if (t.timeoutTimer) {
            t.timeoutTimer->stop();
            t.timeoutTimer->deleteLater();
            t.timeoutTimer = nullptr;
        }
        m_pendingRequests.remove(sessionId);
    }

    emit requestFinished(targetKey);
}

void ChatBotRequestProcessor::onChannelError(const QString &platform, const QString &error)
{
    const QList<QString> pendingIds = m_pendingRequests.keys();
    for (const QString &sessionId : pendingIds) {
        ReplyTarget &t = m_pendingRequests[sessionId];
        if (t.platform != platform)
            continue;

        qCWarning(logChatBot) << "Cancelling session" << sessionId
                                << "due to channel error on" << platform;
        m_sessionMgr->cancelSession(sessionId);

        if (t.timeoutTimer) {
            t.timeoutTimer->stop();
            t.timeoutTimer->deleteLater();
            t.timeoutTimer = nullptr;
        }

        sendFinalReply(t.platform, t.to, t.conversationType, t.streamHandle,
                       tr("Platform error, request cancelled: %1").arg(error));

        const QString targetKey = t.platform + ":" + t.to;
        m_pendingRequests.remove(sessionId);
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

QSharedPointer<ConversationRecord>
ChatBotRequestProcessor::buildConversation(const QString &convKey, const QString &userText) const
{
    QSharedPointer<ConversationRecord> record(new ConversationRecord(GlobalUtil::generateMsId()));
    record->setAssistantId(QString(STR_KEY_UOS_AI_CHATBOT));

    const QJsonArray history = m_memory->buildContext(convKey, userText);
    if (history.isEmpty())
        return {};

    QString prevId;

    // 最后一条为当前用户问题，单独处理
    for (int i = 0; i < history.size() - 1; ++i) {
        const QJsonObject msg  = history[i].toObject();
        const QString     role = msg.value(STR_KEY_ROLE).toString();

        ModelMessage mm;
        mm.role = role;

        if (role == QLatin1String(STR_KEY_ASSISTANT) && msg.contains(STR_KEY_TOOL_CALLS)) {
            // assistant 带工具调用 → CntTool MetaMessage 存 ModelToolCallList
            ModelToolCallList tcList;
            for (const QJsonValue &tc : msg.value(STR_KEY_TOOL_CALLS).toArray()) {
                const QJsonObject tcObj = tc.toObject();
                const QJsonObject fn    = tcObj.value(STR_KEY_FUNCTION).toObject();
                ModelToolCall call;
                call.id        = tcObj.value(STR_KEY_ID).toString();
                call.name      = fn.value(STR_KEY_NAME).toString();
                call.arguments = QJsonDocument::fromJson(
                    fn.value(STR_KEY_ARGUMENTS).toString().toUtf8()
                ).object().toVariantHash();
                tcList.append(call);
            }
            MetaMessage meta;
            meta.type = CntTool;
            meta.data = QVariant::fromValue(tcList);
            mm.content.append(meta);

            const QString text = msg.value(STR_KEY_CONTENT).toString();
            if (!text.isEmpty()) {
                MetaMessage textMeta;
                textMeta.type = CntText;
                textMeta.data = text;
                mm.content.append(textMeta);
            }
        } else if (role == QLatin1String(STR_KEY_TOOL)) {
            // tool result → CntTool MetaMessage 存 {tool_call_id, content}
            QVariantHash toolData;
            toolData[STR_KEY_TOOL_CALL_ID] = msg.value(STR_KEY_TOOL_CALL_ID).toString();
            toolData[STR_KEY_CONTENT]      = msg.value(STR_KEY_CONTENT).toString();
            MetaMessage meta;
            meta.type = CntTool;
            meta.data = toolData;
            mm.content.append(meta);
        } else if (role == QLatin1String(STR_KEY_USER) || role == QLatin1String(STR_KEY_ASSISTANT)) {
            const QString content = msg.value(STR_KEY_CONTENT).toString();
            if (content.isEmpty())
                continue;
            MetaMessage meta;
            meta.type = CntText;
            meta.data = content;
            mm.content.append(meta);
        } else {
            continue;
        }

        if (mm.content.isEmpty())
            continue;

        MessageNodePtr node(new MessageNode);
        node->setId(GlobalUtil::generateMsId());
        node->setRole(role == QLatin1String(STR_KEY_USER) ? MrUser : MrAssistant);
        node->appendMessage(mm);

        if (!record->addMessage(prevId, node))
            return {};
        prevId = node->getId();
    }

    // 当前用户问题作为 currentMessage
    const QString curId = GlobalUtil::generateMsId();
    MessageNodePtr curNode(new MessageNode);
    curNode->setId(curId);
    curNode->setRole(MrUser);

    ModelMessage mm;
    mm.role = STR_KEY_USER;
    MetaMessage meta;
    meta.type = CntText;
    meta.data = userText;
    mm.content.append(meta);
    curNode->appendMessage(mm);

    if (!record->addMessage(prevId, curNode))
        return {};

    record->setCurrentMessage(curId);
    return record;
}
