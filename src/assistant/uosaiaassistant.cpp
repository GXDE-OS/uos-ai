#include "uosaiaassistant.h"
#include "global_key_define.h"
#include "model/modelvendor.h"
#include "agent/generic/genericagent.h"
#include "agent/generic/onlinesearchagent.h"
#include "conversation/conversationrecord.h"
#include "model/builtinprovider.h"
#include "services/fileservice/fileservice.h"
#include "network/httpcodetranslation.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

UOSAIAssistant::UOSAIAssistant(QObject *parent) : AbstractAssistant(parent)
{
}

UOSAIAssistant::~UOSAIAssistant()
{
}

void UOSAIAssistant::cancel()
{
    canceled = true;
    emit requestCancel();
}

QVariantHash UOSAIAssistant::run()
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
    } else {
        // 判断是否免费账号
        if (ModelVendor::isUosProvider(account)) {
            // 是否为智能调度
            if (account->model.id == UOS_FREE_MODEL_AUTO) {
                // 改为使用glm 4.7
                account->model = BuiltinProvider::instance()->getModelInfo(UOS_FREE_GLM_4_7);
            }
        }
    }

    // 处理消息
    QList<ModelMessage> historyMsg;
    ModelMessage currentMessage;
    processMessage(currentMessage, historyMsg, m_parameters.value(STR_KEY_RETRY, false).toBool());

    // 如果开启了在线搜索，先使用 OnlineSearchAgent
    QString searchContext;
    if (m_parameters.value(STR_KEY_ONLINE).toBool())
        searchContext = runOnlineSearch(account, currentMessage, historyMsg);

    // 使用 GenericAgent 处理请求
    QVariantHash response = runGenericAgent(account, currentMessage, historyMsg, searchContext);

    if (response.contains(STR_KEY_CONTENT))
        result[STR_KEY_CONTENT] = response.value(STR_KEY_CONTENT);

    if (response.contains(STR_KEY_CONTEXT))
        result[STR_KEY_CONTEXT] = response.value(STR_KEY_CONTEXT);

    return result;
}

void UOSAIAssistant::processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry)
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

QString UOSAIAssistant::runOnlineSearch(ModelAccountPtr account, const ModelMessage &currentMessage, const QList<ModelMessage> &historyMsg)
{
    auto modelVendor = ModelVendor::instance();

    QScopedPointer<OnlineSearchAgent> searchAgent(new OnlineSearchAgent);
    connect(this, &UOSAIAssistant::requestCancel, searchAgent.data(), &LlmAgent::cancel, Qt::DirectConnection);
    searchAgent->initialize();

    if (canceled) {
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_HTTP_ERROR] = QNetworkReply::NetworkError::OperationCanceledError;
        m_error[STR_KEY_ERROR_MESSAGE] = HttpCodeTranslation::translation(QNetworkReply::NetworkError::OperationCanceledError, "");
        return QString();
    }

    auto searchModel = modelVendor->createModel(account).dynamicCast<AbstractChatModel>();
    if (searchModel.isNull()) {
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "Failed to create model";
        qCWarning(logAssistant) << "Failed to create search model for account:" << account->id;
        return QString();
    }

    searchAgent->setModel(searchModel);

    // 设置搜索参数：关闭流式传输，关闭深度思考
    QVariantHash searchParams;
    searchParams[STR_KEY_STREAM] = false;
    searchParams[STR_KEY_THINKING] = false;
    searchAgent->setModelParams(searchParams);

    connect(searchAgent.data(), &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
            qCDebug(logAssistant) << "render: " << strData;
        }
    }, Qt::DirectConnection);

    QVariantHash agentParams;
    if (account->account.provider.compare(STR_KEY_MODELHUB) == 0) {
        agentParams[STR_KEY_MAX_TOKENS] = 4000;
    }

    // 执行搜索
    QVariantHash searchResponse = searchAgent->processRequest(currentMessage, historyMsg, agentParams);
    qCDebug(logAssistant) << "OnlineSearchAgent processRequest response:" << searchResponse;

    // 提取搜索结果
    QString searchContext = searchResponse.value("search_content").toString();
    return searchContext;
}

QVariantHash UOSAIAssistant::runGenericAgent(ModelAccountPtr account, const ModelMessage &currentMessage, const QList<ModelMessage> &historyMsg, const QString &searchContext)
{
    auto modelVendor = ModelVendor::instance();

    QScopedPointer<GenericAgent> agent(new GenericAgent);
    connect(this, &UOSAIAssistant::requestCancel, agent.data(), &LlmAgent::cancel, Qt::DirectConnection);
    agent->initialize();

    if (canceled) {
        m_error[STR_KEY_ERROR] = GErrorType::HttpError;
        m_error[STR_KEY_HTTP_ERROR] = QNetworkReply::NetworkError::OperationCanceledError;
        m_error[STR_KEY_ERROR_MESSAGE] = HttpCodeTranslation::translation(QNetworkReply::NetworkError::OperationCanceledError, "");
        return {};
    }

    auto model = modelVendor->createModel(account).dynamicCast<AbstractChatModel>();
    if (model.isNull()) {
        m_error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        m_error[STR_KEY_MESSAGE] = "Failed to create model";
        qCWarning(logAssistant) << "Failed to create model for account:" << account->id;
        return QVariantHash();
    }

    agent->setModel(model);

    QVariantHash modelParams;
    modelParams[STR_KEY_STREAM] = true;
    modelParams[STR_KEY_THINKING] = m_parameters.value(STR_KEY_THINKING, false).toBool();
    agent->setModelParams(modelParams);

    agent->setSearchedContent(searchContext);

    connect(agent.data(), &LlmAgent::messageReceived, this, [this](const RenderMessageList &msgs) {
        for (const auto &msg : msgs) {
            auto strData = QString::fromUtf8(QJsonDocument(msg.toJson()).toJson(QJsonDocument::Compact));
            emit pushMessage(strData);
            qCDebug(logAssistant) << "render: " << strData;
        }
    }, Qt::DirectConnection);

    QVariantHash response = agent->processRequest(currentMessage, historyMsg);
    qCDebug(logAssistant) << "LlmAgent processRequest response:" << response;

    m_error = agent->lastError();

    return response;
}
