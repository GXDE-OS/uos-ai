#include "llmagent.h"
#include "llm.h"
#include "oaifunctionparser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDir>
#include <QDate>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

LlmAgent::LlmAgent(QObject *parent)
    : QObject(parent)
    , Agent()
{

}

bool LlmAgent::initialize()
{
    return true;
}

void LlmAgent::setModel(QSharedPointer<LLM> llm)
{
    m_llm = llm;
}

QJsonObject LlmAgent::processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params)
{
    QJsonObject response;
    
    if (!m_llm) {
        qCWarning(logAgent) << "No model set for LlmAgent";
        response["error"] = "No model set";
        return response;
    }
    
    auto messages = initChatMessages(question, history);

    QJsonArray context;

    while (!canceled) {
        if (m_llm->stream()) {
            connect(m_llm.data(), &LLM::readyReadChatDeltaContent, this, [this](const QString &content) {
                if (canceled) {
                    qCDebug(logAgent()) << "received delta content after abort." << content;
                    return;
                }

                OutputCtx ctx;
                ctx.deltaContent = content;
                bool ret = handleStreamOutput(&ctx);
                if (!ret) {
                    canceled = true;
                    // cancle
                    m_llm->aborted();
                }
            });
        }

        QJsonObject output = m_llm->predict(QJsonDocument(messages).toJson(), m_tools);
        disconnect(m_llm.data(), &LLM::readyReadChatDeltaContent, this, nullptr);

        OAIFunctionParser parser;
        if (parser.hasToolCalls(output)) {
            auto tools = parser.parseResponse(output);
            if (tools.isEmpty())
                break;

            // 使用OAIFunctionParser创建OAI格式的assistant消息
            QJsonObject assistantMsg = OAIFunctionParser::createAssistantMessage(tools);
            context.append(assistantMsg);
            messages.append(context.last());
            
            bool isAbort = false;
            // 调用callTool接口并添加tool结果到聊天记录
            for (const auto &toolCall : tools) {
                qCInfo(logAgent) << "Agent" << name() << "calling tool:" << toolCall.name << toolCall.arguments;
                auto result = callTool(toolCall.name, toolCall.arguments);
                if (result.first < 0)
                    isAbort = true;

                qCDebug(logAgent) << "Agent" << name() << "tool" << toolCall.name << "output:" << result.second << result.first;
                // 使用OAIFunctionParser创建tool消息
                QJsonObject toolMsg = OAIFunctionParser::createToolMessage(toolCall, result.second);
                context.append(toolMsg);
                messages.append(context.last());
            }

            if (isAbort) {
                qCWarning(logAgent) << "break flow by calling tool.";
                break;
            }

            continue;
        }

        context.append(QJsonObject{{"role","assistant"}, {"content", output.value("content").toString()}});
        // final output
        response["content"] = output.value("content").toString();
        break;
    }

    response["context"] = context;
    return response;
}

void LlmAgent::cancel()
{
    qCInfo(logAgent) << "LLM agent request canceled";
    canceled = true;
}

QJsonArray LlmAgent::initChatMessages(const QJsonObject &question, const QJsonArray &history) const
{
    QJsonArray initialMessages;
    
    // 添加系统消息
    {
        QString prompt = systemPrompt();
        if (!prompt.isEmpty()) {
            QJsonObject systemMessage;
            systemMessage["role"] = "system";
            systemMessage["content"] = prompt;
            initialMessages.append(systemMessage);
        }
    }
    
    // 添加历史消息，过滤掉system角色的消息
    for (const QJsonValue &msg : history) {
        QJsonObject msgObj = msg.toObject();
        if (msgObj["role"].toString() != "system") {
            initialMessages.append(msg);
        }
    }
    
    // 添加当前用户问题
    {
        QJsonObject userMessage;
        userMessage["role"] = "user";
        userMessage["content"] = question["content"].toString();
        initialMessages.append(userMessage);
    }
    
    return initialMessages;
}

bool LlmAgent::handleStreamOutput(OutputCtx *ctx)
{
    emit readyReadChatDeltaContent(ctx->deltaContent);
    return true;
}

QPair<int, QString> LlmAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    // 默认实现：返回错误状态，表示工具调用不可用
    qCWarning(logAgent) << "Tool calling not implemented in base LlmAgent class:" << toolName;
    return qMakePair(-1, QString("Tool calling not implemented"));
}

void LlmAgent::textChainContent(const QString &content)
{
    QJsonObject message;

    message.insert("content", content);
    message.insert("chatType", ChatAction::ChatTextPlain);  // 普通文本类型

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

} // namespace uos_ai 
