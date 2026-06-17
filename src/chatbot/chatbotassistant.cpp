#include "chatbotassistant.h"
#include "chatbotagent.h"

#include "global_key_define.h"
#include "chatbot_key_define.h"
#include "conversation/conversationrecord.h"
#include "model/modelvendor.h"

#include <QJsonDocument>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;
using namespace uos_ai::chatbot;

ChatBotAssistant::ChatBotAssistant(QObject *parent)
    : AbstractAssistant(parent)
{
}

void ChatBotAssistant::cancel()
{
    emit requestCancel();
}

QVariantHash ChatBotAssistant::run()
{
    QVariantHash result;

    if (!m_conversation) {
        m_error[STR_KEY_ERROR]   = GErrorType::InvalidAssistant;
        m_error[STR_KEY_MESSAGE] = "No conversation set";
        return result;
    }

    QScopedPointer<ChatbotAgent> agent(new ChatbotAgent);
    connect(this, &ChatBotAssistant::requestCancel, agent.data(), &LlmAgent::cancel, Qt::DirectConnection); // 必须 DirectConnection

    agent->initialize();

    auto modelVendor       = ModelVendor::instance();
    ModelAccountPtr account = modelVendor->getModel(m_modelId);
    if (!account.constData()) {
        qCWarning(logAssistant) << "ChatBotAssistant: no model found for id:" << m_modelId;
        m_error[STR_KEY_ERROR]   = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "No model found for id: " + m_modelId;
        return result;
    }

    auto model = modelVendor->createModel(account).dynamicCast<AbstractChatModel>();
    if (model.isNull()) {
        m_error[STR_KEY_ERROR]   = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "Failed to create model";
        qCWarning(logAssistant) << "ChatBotAssistant: failed to create model for account:" << account->id;
        return result;
    }

    agent->setModel(model);

    QVariantHash modelParams;
    modelParams[STR_KEY_STREAM]   = true;
    modelParams[STR_KEY_THINKING] = m_parameters.value(STR_KEY_THINKING, false).toBool();
    agent->setModelParams(modelParams);

    QList<ModelMessage> historyMsg;
    ModelMessage currentMessage;
    processMessage(currentMessage, historyMsg, m_parameters.value(STR_KEY_RETRY, false).toBool());

    connect(agent.data(), &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
        }
    }, Qt::DirectConnection);

    QVariantHash agentParams;
    agentParams[STR_KEY_MCP_SERVERS]                         = m_parameters.value(STR_KEY_MCP_SERVERS);
    agentParams[QLatin1String(kChatbotPlatformParam)]        = m_parameters.value(QLatin1String(kChatbotPlatformParam));
    agentParams[QLatin1String(kChatbotHistorySummaryParam)]  = m_parameters.value(QLatin1String(kChatbotHistorySummaryParam));

    QVariantHash response = agent->processRequest(currentMessage, historyMsg, agentParams);

    m_error = agent->lastError();

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}

void ChatBotAssistant::processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry)
{
    QString currentMsgId = m_conversation->currentMessage();

    QList<MessageNodePtr> history = m_conversation->history(currentMsgId);
    Q_ASSERT(!history.isEmpty());

    MessageNodePtr question = history.takeLast();
    Q_ASSERT(question->getId() == currentMsgId);

    for (auto node : history)
        historyMsg.append(node->getMessage());

    auto qmsg = question->getMessage();
    Q_ASSERT(!qmsg.isEmpty());

    currentMessage = qmsg.takeLast();

    if (retry) {
        historyMsg.append(qmsg);
        return;
    }
}
