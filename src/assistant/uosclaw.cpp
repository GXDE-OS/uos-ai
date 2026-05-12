#include "uosclaw.h"
#include "mcp/defaultagentwithskills.h"
#include "global_key_define.h"
#include "model/modelvendor.h"
#include "agent/generic/genericagent.h"
#include "conversation/conversationrecord.h"
#include "dconfigmanager.h"
#include "agent/mcpserver.h"
#include "services/fileservice/fileservice.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

UOSClaw::UOSClaw(QObject *parent) : AbstractAssistant(parent)
{

}

void UOSClaw::cancel()
{
    emit requestCancel();
}

QString UOSClaw::faq()
{
    QStringList questions = {
        tr("Get system memory usage"),
        tr("Get system CPU usage"),
        tr("Get system disk information"),
        tr("Switch to dark theme"),
        tr("Switch to light theme"),
        tr("Change desktop background"),
        tr("Switch dock mode"),
        tr("Enable eye protection mode")
    };

    std::shuffle(questions.begin(), questions.end(), std::mt19937{std::random_device{}()});

    QJsonArray faqArray;
    // 随机6个问题
    auto randomQuestions = questions.mid(0, qMin(6, questions.size()));
    for (const QString &question : randomQuestions) {
        QJsonObject obj;
        obj["Question"] = question;
        faqArray.append(obj);
    }

    return QJsonDocument(faqArray).toJson(QJsonDocument::Compact);
}

QVariantHash UOSClaw::run()
{
    QVariantHash result;

    if (!m_conversation) {
        result[STR_KEY_ERROR] = GErrorType::InvalidAssistant;
        result[STR_KEY_MESSAGE] = "No conversation set";
        return result;
    }

    QScopedPointer<DefaultAgentWithSkills> agent(new DefaultAgentWithSkills);
    connect(this, &UOSClaw::requestCancel, agent.data(), &DefaultAgent::cancel, Qt::DirectConnection); // must be DirectConnection

    agent->initialize();

    auto modelVendor = ModelVendor::instance();
    ModelAccountPtr account = modelVendor->getModel(m_modelId);

    if (!account.constData()) {
        qCWarning(logAssistant) << "No model found for id: " + m_modelId;
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "No model found for id: " + m_modelId;
        return result;
    }

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

    QVariantHash agentParams;
    agentParams["mcpServers"] = m_parameters.value("mcpServers");

    QVariantHash response = agent->processRequest(currentMessage, historyMsg, agentParams);
    qCDebug(logAssistant) << "DefaultAgent processRequest response:" << response;

    m_error = agent->lastError();

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}

void UOSClaw::processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry)
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
            // TODO 等正式的协议
            for (const QVariant &tmp  :meta.data.toList()) {
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
            // TODO 等正式的协议
            for (const QVariant &tmp  :meta.data.toList()) {
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
    for (const QString &content: files) {
        user.append("\n\n Attachment file:\n");
        user.append(content);
    }

    for (const QString &content: images) {
        user.append("\n\n Attachment image:\n");
        user.append(content);
    }

    currentMessage.content.clear();

    // 构造新的消息
    {
        MetaMessage meta;
        meta.type = CntText;
        meta.data = user;
        currentMessage.content.append(meta);
    }

    // 更新聊天记录
    {
        qmsg.append(currentMessage);
        question->setMessage(qmsg);
    }
 }
