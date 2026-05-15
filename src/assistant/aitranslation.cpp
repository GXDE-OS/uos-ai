#include "aitranslation.h"
#include "agent/translation/translationagent.h"
#include "global_key_define.h"
#include "global_define.h"
#include "model/modelvendor.h"
#include "conversation/conversationrecord.h"
#include "services/fileservice/fileservice.h"

#include <QLoggingCategory>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

AITranslation::AITranslation(QObject *parent) : AbstractAssistant(parent)
{

}

QString AITranslation::getTranslationFAQ()
{
    // 翻译助手 FAQ 问题列表
    const QStringList questions = {
        tr("Translate the following text into English for me."),
        tr("Translate the following document into Chinese."),
        tr("What does the word \u201cAgent\u201d mean in the AI industry?"),
        tr("Please translate the following content into Chinese. Requirements: Accurate in meaning, formal and professional in language."),
        tr("What are some colloquial ways to address a friend in English?"),
        tr("Translate the following classical Chinese text into modern Chinese."),
    };

    QJsonArray faqArray;
    for (const QString &question : questions) {
        QJsonObject obj;
        obj["Question"] = question;
        faqArray.append(obj);
    }

    return QJsonDocument(faqArray).toJson(QJsonDocument::Compact);
}

AITranslation::~AITranslation()
{
}

void AITranslation::cancel()
{
    emit requestCancel();
}

QVariantHash AITranslation::run()
{
    QVariantHash result;

    if (!m_conversation) {
        result[STR_KEY_ERROR] = GErrorType::InvalidAssistant;
        result[STR_KEY_MESSAGE] = "No conversation set";
        m_error = result;
        return result;
    }

    QScopedPointer<TranslationAgent> agent(new TranslationAgent);
    connect(this, &AITranslation::requestCancel, agent.data(), &TranslationAgent::cancel, Qt::DirectConnection); // must be DirectConnection

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
    processMessage(currentMessage, m_parameters.value(STR_KEY_RETRY, false).toBool());

    connect(agent.data(), &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
            qCDebug(logAssistant) << "render: " << strData;
        }
    }, Qt::DirectConnection);

    QVariantHash response = agent->processRequest(currentMessage, historyMsg);
    qCDebug(logAssistant) << "TranslationAgent processRequest response:" << response;

    m_error = agent->lastError();

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}

void AITranslation::processMessage(ModelMessage &currentMessage, bool retry)
{
    QString currentMsgId = m_conversation->currentMessage();

    QList<MessageNodePtr> history = m_conversation->history(currentMsgId);
    Q_ASSERT(!history.isEmpty());

    MessageNodePtr question = history.takeLast();
    Q_ASSERT(question->getId() == currentMsgId);

    auto qmsg = question->getMessage();
    Q_ASSERT(!qmsg.isEmpty());

    currentMessage = qmsg.takeLast();

    // 重试无需再处理消息
    if (retry) {
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

    // 将附件内容追加到用户问题中，翻译智能体直接翻译文件内容，不需要额外的字段
    for (const QString &content: files) {
        user.append(content);
    }

    for (const QString &content: images) {
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
