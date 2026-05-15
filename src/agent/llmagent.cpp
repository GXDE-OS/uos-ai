#include "llmagent.h"
#include "model/abstractchatmodel.h"
#include "global_key_define.h"
#include "modeltool.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QDate>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QThread>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

static bool isRetryable(QNetworkReply::NetworkError e) {
    switch (e) {
        case QNetworkReply::ConnectionRefusedError:
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::HostNotFoundError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::InternalServerError:
        case QNetworkReply::ServiceUnavailableError:
            return true;
        default:
            return false;
    }
};

LlmAgent::LlmAgent(QObject *parent)
    : QObject(parent)
    , Agent()
{

}

LlmAgent::~LlmAgent()
{
    // FIXME, 不断连会崩溃
    if (m_llm)
        disconnect(m_llm.data(), nullptr, this, nullptr);
}

bool LlmAgent::initialize()
{
    return true;
}

void LlmAgent::setModel(QSharedPointer<AbstractChatModel> llm)
{
    m_llm = llm;
}

void LlmAgent::setModelParams(const QVariantHash &params)
{
    m_modelParams = params;
}

QVariantHash LlmAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &agentParams)
{
    QVariantHash response;

    if (!m_llm) {
        qCWarning(logAgent) << "No model set for LlmAgent";
        return response;
    }

    auto messages = initChatMessages(question, history);

    QList<ModelMessage> context;
    int chatTurn =  0;
    while (!canceled) {
        bool received = false;
        chatTurn++;

        if (m_modelParams.value(STR_KEY_STREAM, false).toBool()) {
            connect(m_llm.data(), &AbstractChatModel::messageReceived, this, [this, &received, chatTurn](const MetaMessageList &msgs) {
                if (canceled) {
                    qCDebug(logAgent) << "received delta content after abort.";
                    return;
                }

                if (!received) {
                    received = true;
                    qCDebug(logAgent) << "this turn received the first chunck." << name() << chatTurn;
                }

                bool ret = handleStreamOutput(msgs);
                if (!ret) {
                    canceled = true;
                    // cancel
                    m_llm->cancel();
                }
            });
        }

        if (!m_tools.isEmpty())
            m_modelParams[STR_KEY_TOOLS] = QVariant::fromValue(m_tools);
        else
            m_modelParams.remove(STR_KEY_TOOLS);

        QVariantHash output;
        // 重试三次
        for (int i = 0; i < 3; ++i) {
            output = m_llm->chatCompletion(messages, m_modelParams);

            auto err = static_cast<QNetworkReply::NetworkError>(m_llm->lastError().value(STR_KEY_HTTP_ERROR).toInt());
            if (isRetryable(err) && !received && output.isEmpty()) {
                qCWarning(logAgent) << "model request error: "<< err << "retry" << i;
                // 增加等待重试
                for (int j = 0; j < (2 + i * 8); ++j) {
                    QThread::msleep(500);
                    qCDebug(logAgent) << "waiting for retry " << j << "check cancel" << canceled;
                    if (canceled)
                        break;
                }

                if (!canceled)
                    continue;

                qCInfo(logAgent) << "user cancel the request when retry";
            }

            break;
        }

        disconnect(m_llm.data(), &AbstractChatModel::messageReceived, this, nullptr);

        ModelMessage curMsg;
        {
            curMsg.role = STR_KEY_ASSISTANT;
            curMsg.source = name();
            curMsg.content = MetaMessage::fromHash(output);

            context.append(curMsg);
            messages.append(curMsg);
        }

        if (!m_llm->lastError().isEmpty()) {
            response[STR_KEY_CONTEXT] = QVariant::fromValue(context);
            response[STR_KEY_CONTENT] = QVariant::fromValue(curMsg);
            return response;
        }

        if (output.contains(STR_KEY_TOOL_CALLS)) {
            ModelToolCallList tools = output.value(STR_KEY_TOOL_CALLS).value<ModelToolCallList>();
            if (tools.isEmpty())
                break;

            bool isAbort = false;
            // 调用callTool接口并添加tool结果到聊天记录
            for (const auto &toolCall : tools) {
                if (canceled)
                    break;
                qCInfo(logAgent) << "Agent" << name() << "calling tool:" << toolCall.name << toolCall.arguments;
                auto result = callTool(toolCall.name, QJsonObject::fromVariantHash(toolCall.arguments));
                if (result.first < 0)
                    isAbort = true;
                qCInfo(logAgent) << "Agent" << name() << "tool" << toolCall.name << "output:" << result.second << result.first;

                ModelMessage toolMsg = ModelMessage::toolOutput(toolCall.id, result.second);
                toolMsg.source = name();

                context.append(toolMsg);
                messages.append(toolMsg);
            }
            if (isAbort) {
                qCWarning(logAgent) << "break flow by calling tool.";
                response[STR_KEY_CONTENT] = QVariant::fromValue(curMsg);
                response[STR_KEY_CONTEXT] = QVariant::fromValue(context);
                break;
            }
            continue;
        }

        // final output
        response[STR_KEY_CONTENT] = QVariant::fromValue(curMsg);
        response[STR_KEY_CONTEXT] = QVariant::fromValue(context);
        break;
    }

    return response;
}

QVariantHash LlmAgent::lastError() const
{
    if (m_llm.isNull()) {
        QVariantHash error;
        error[STR_KEY_ERROR] = GErrorType::InvalidModel;
        error[STR_KEY_MESSAGE] = "No model set for LlmAgent";
        return error;
    }

    return m_llm->lastError();
}

void LlmAgent::cancel()
{
    qCInfo(logAgent) << "LLM agent request canceled";
    canceled = true;

    if (m_llm)
        m_llm->cancel();
}

QList<ModelMessage> LlmAgent::initChatMessages(const ModelMessage &question, const QList<ModelMessage> &history) const
{
    QList<ModelMessage> initialMessages;

    // 添加系统消息
    {
        QString prompt = systemPrompt();
        if (!prompt.isEmpty()) {
            ModelMessage systemMessage;
            systemMessage.role = STR_KEY_SYSTEM;
            systemMessage.content.append({ContentType::CntText, prompt});
            initialMessages.append(systemMessage);
        }
    }

    // 添加历史消息，过滤掉system角色的消息
    for (const ModelMessage &msg : history) {
        if (msg.role.compare(STR_KEY_SYSTEM) != 0) {
            initialMessages.append(msg);
        }
    }

    // 添加当前用户问题
    initialMessages.append(question);

    return initialMessages;
}

bool LlmAgent::handleStreamOutput(const MetaMessageList &msgs)
{
    RenderMessageList rmsgs;
    //todo 协议转换
    for (const  MetaMessage &tmp : msgs) {
        RenderMessage rmsg;
        rmsg.type = tmp.type;
        rmsg.data = tmp.data;
        rmsgs.append(rmsg);
    }

    emit messageReceived(rmsgs);
    return true;
}

QPair<int, QString> LlmAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    // 默认实现：返回错误状态，表示工具调用不可用
    qCWarning(logAgent) << "Tool calling not implemented in base LlmAgent class:" << toolName;
    return qMakePair(-1, QString("Tool calling not implemented"));
}


} // namespace uos_ai 
