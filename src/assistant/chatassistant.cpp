#include "chatassistant.h"
#include "global_key_define.h"
#include "model/modelvendor.h"
#include "agent/generic/chatagent.h"
#include "conversation/conversationrecord.h"

#include <QJsonDocument>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)
using namespace uos_ai;

ChatAssistant::ChatAssistant(QObject *parent) : AbstractAssistant(parent)
{

}

void ChatAssistant::cancel()
{
    emit requestCancel();
}

QVariantHash ChatAssistant::run()
{
    QVariantHash result;

    if (!m_conversation) {
        m_error[STR_KEY_ERROR] = GErrorType::InvalidAssistant;
        m_error[STR_KEY_MESSAGE] = "No conversation set";
        return result;
    }

    auto modelVendor = ModelVendor::instance();
    ModelAccountPtr account = modelVendor->getModel(m_modelId);

    if (!account.constData()) {
        qCWarning(logAssistant) << "No model found for id: " + m_modelId;
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "No model found for id: " + m_modelId;
        return result;
    }

    QScopedPointer<LlmAgent> agent(new ChatAgent);

    connect(this, &ChatAssistant::requestCancel, agent.data(), &LlmAgent::cancel, Qt::DirectConnection); // must be DirectConnection
    agent->initialize();

    auto model = modelVendor->createModel(account).dynamicCast<AbstractChatModel>();
    if (model.isNull()) {
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "Failed to create model";
        qCWarning(logAssistant) << "Failed to create model for account:" << account->id;
        return result;
    }

    agent->setModel(model);

    QVariantHash modelParams;
    modelParams[STR_KEY_STREAM] = m_parameters.value(STR_KEY_STREAM, true); //默认流
    modelParams[STR_KEY_THINKING] = m_parameters.value(STR_KEY_THINKING);
    agent->setModelParams(modelParams);

    QList<ModelMessage> historyMsg;
    ModelMessage currentMessage;
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
    }

    connect(agent.data(), &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
            qCDebug(logAssistant) << "render: " << strData;
        }
    }, Qt::DirectConnection);

    QVariantHash response = agent->processRequest(currentMessage, historyMsg);
    qCDebug(logAssistant) << "chat agent processRequest response:" << response;

    m_error = agent->lastError();

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}
