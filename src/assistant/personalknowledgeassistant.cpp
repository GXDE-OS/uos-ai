#include "personalknowledgeassistant.h"
#include "agent/knowledge/knowledgebaseagent.h"
#include "model/modelvendor.h"
#include "conversation/conversationrecord.h"
#include "global_key_define.h"
#include "dbus/embeddingserver.h"
#include "localmodelserver.h"
#include "services/fileservice/fileservice.h"

#include <QJsonDocument>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

PersonalKnowledgeAssistant::PersonalKnowledgeAssistant(QObject *parent) : AbstractAssistant(parent)
{
}

PersonalKnowledgeAssistant::~PersonalKnowledgeAssistant()
{
}

void PersonalKnowledgeAssistant::cancel()
{
    emit requestCancel();
}

QVariantHash PersonalKnowledgeAssistant::run()
{
    QVariantHash result;

    if (!m_conversation) {
        result[STR_KEY_ERROR] = GErrorType::InvalidAssistant;
        result[STR_KEY_MESSAGE] = "No conversation set";
        return result;
    }

    // 检查向量化插件和知识库状态
    if (!LocalModelServer::getInstance().checkInstallStatus(PLUGINSNAME)) {
        m_error[STR_KEY_ERROR] = GErrorType::KnowledgeBasePluginNotInstalled;
        m_error[STR_KEY_ERROR_MESSAGE] = tr("The Personal Knowledge Assistant can only be used after configuring the model plug.");
        return result;
    }

    if (EmbeddingServer::getInstance().getDocFiles().isEmpty()) {
        m_error[STR_KEY_ERROR] = GErrorType::KnowledgeBaseEmpty;
        m_error[STR_KEY_ERROR_MESSAGE] = tr("The Personal Knowledge Assistant can only be used after configuring the knowledge base.");
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

    QScopedPointer<KnowledgeBaseAgent> agent(new KnowledgeBaseAgent);
    connect(this, &PersonalKnowledgeAssistant::requestCancel, agent.data(), &LlmAgent::cancel, Qt::DirectConnection); // must be DirectConnection
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
    modelParams[STR_KEY_STREAM] = true;
    modelParams[STR_KEY_THINKING] = m_parameters.value(STR_KEY_THINKING);
    agent->setModelParams(modelParams);

    QList<ModelMessage> historyMsg;
    ModelMessage currentMessage;
    processMessage(currentMessage, historyMsg, m_parameters.value(STR_KEY_RETRY, false).toBool());

    connect(agent.data(), &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
            qCDebug(logAssistant) << "render: " << strData;
        }
    }, Qt::DirectConnection);

    QVariantHash response = agent->processRequest(currentMessage, historyMsg);
    qCDebug(logAssistant) << "KnowledgeBaseAgent processRequest response:" << response;

    m_error = agent->lastError();

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}

void PersonalKnowledgeAssistant::processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry)
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

    // 重试无需再处理消息
    if (retry) {
        historyMsg.append(qmsg);
        return;
    }

    // 处理附件
    QString user;
    QStringList files;
    QStringList images;

    for (const MetaMessage &meta : currentMessage.content) {
        if (meta.type == CntFile) {
            for (const QVariant &tmp : meta.data.toList()) {
                QString path = tmp.toString();
                QString content = FileService::instance()->cachedContent(path);
                if (content.isEmpty()) {
                    qCWarning(logAssistant) << "Failed to read file content for path: " << path;
                    continue;
                }
                files.append(content);
            }
        } else if (meta.type == CntText) {
            user.append(meta.data.toString());
        } else if (meta.type == CntImage) {
            for (const QVariant &tmp : meta.data.toList()) {
                QString path = tmp.toString();
                QString content = FileService::instance()->cachedContent(path);
                if (content.isEmpty()) {
                    qCWarning(logAssistant) << "Failed to read image content for path: " << path;
                    continue;
                }
                images.append(content);
            }
        } else {
            qCWarning(logAssistant) << "unsupported meta type: " << meta.type;
        }
    }

    if (files.isEmpty() && images.isEmpty())
        return;

    // 将附件内容追加到用户问题中
    for (const QString &content : files) {
        user.append("\n\n Attachment file:\n");
        user.append(content);
    }

    for (const QString &content : images) {
        user.append("\n\n Attachment image:\n");
        user.append(content);
    }

    currentMessage.content.clear();

    {
        MetaMessage meta;
        meta.type = CntText;
        meta.data = user;
        currentMessage.content.append(meta);
    }

    {
        qmsg.append(currentMessage);
        question->setMessage(qmsg);
    }
}
