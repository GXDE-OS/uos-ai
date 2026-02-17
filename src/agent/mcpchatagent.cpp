#include "mcpchatagent.h"
#include "networkdefs.h"
#include "mcpclient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QCoreApplication>
#include <QThread>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

MCPChatAgent::MCPChatAgent(QObject *parent)
    : MCPAgent(parent)
{
}

MCPChatAgent::~MCPChatAgent()
{
}

QJsonObject MCPChatAgent::processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params)
{
    QJsonObject response;
    
    // 先刷新一次服务
    syncServers();

    // 取消
    if (canceled) {
        qCDebug(logAgent) << "agent canceled before fetch tools.";
        return response;
    }

    {
        auto toolRet = fetchTools(params.value(PREDICT_PARAM_MCPSERVERS).toStringList());
        if (toolRet.first != 0) {
            qCWarning(logAgent) << "Failed to fetch tools:" << toolRet.second;
            if (m_llm) {
                m_llm->setLastError(toolRet.first);
                m_llm->setLastErrorString(toolRet.second);
            }
            return response;
        }
    }

    // 取消
    if (canceled){
        qCDebug(logAgent) << "agent canceled before init messages.";
        return response;
    }

    auto messages = initChatMessages(question, history);
    QJsonArray context;

    while (!canceled) {
        QString current_output;
        QString tool_results;
        ToolParser parser;

        connect(m_llm.data(), &LLM::readyReadChatDeltaContent, this, [&current_output, &tool_results, &parser, this](const QString &content) {
            if (canceled) {
                qCDebug(logAgent()) << "received delta content after abort." << content;
                return;
            }
            MCPChatCtx ctx;
            ctx.deltaContent = content;
            ctx.content = &current_output;
            ctx.toolResults = &tool_results;
            ctx.parser = &parser;

            bool ret = handleStreamOutput(&ctx);
            if (!ret) {
                canceled = true;
                // cancel
                m_llm->aborted();
            }
        });

        auto output = m_llm->predict(QJsonDocument(messages).toJson(), {});

        disconnect(m_llm.data(), &LLM::readyReadChatDeltaContent, this, nullptr);

        if (!canceled) {
            parser.finalize();
            for (auto action : parser.getResults()) {
                if (action.first == ToolParser::Text) {
                    if (!action.second.isEmpty())
                        this->textChainContent(action.second);
                } else if (action.first == ToolParser::Function) {
                    qCritical(logAgent) << "Catch function in end:" << action.second;
                }
            }
        }

        if (tool_results.isEmpty()) {
            qCInfo(logAgent) << "No tool results, end chat. messages size:" << messages.size();
            response["content"] = output.value("content").toString();
            break;
        }

        messages.append(QJsonObject{{"role","assistant"}, {"content", current_output}});
        context.append(messages.last());

        messages.append(QJsonObject{{"role", "user"}, {"content",tool_results}});
        context.append(messages.last());
    }

    response["context"] = context;
    return response;
}

void MCPChatAgent::textChainContent(const QString &content)
{
    QJsonObject message;

    message.insert("content", content);
    message.insert("chatType", ChatAction::ChatTextPlain);  // 普通文本类型

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

void MCPChatAgent::toolUseContent(const ToolUse &tool)
{
    QJsonObject message = tool.toJson();
    message.insert("chatType", ChatAction::ChatToolUse);

    QJsonObject wrapper;
    wrapper.insert("message", message);
    wrapper.insert("stream", true);

    emit readyReadChatDeltaContent(QJsonDocument(wrapper).toJson());
}

bool MCPChatAgent::handleStreamOutput(OutputCtx *ctx)
{
    auto mctx = dynamic_cast<MCPChatCtx *>(ctx);
    Q_ASSERT(mctx);

    QString realContent;
    QJsonDocument doc = QJsonDocument::fromJson(ctx->deltaContent.toUtf8());
     if (!doc.isNull() && doc.isObject()) {
         QJsonObject obj = doc.object();
         if (obj.contains("message") && obj["message"].isObject()) {
             QJsonObject messageObj = obj["message"].toObject();
             if (messageObj.value("chatType").toInt() != ChatAction::ChatTextPlain) {
                 emit readyReadChatDeltaContent(ctx->deltaContent);
                 return true;
             }

             if (messageObj.contains("content")) {
                 realContent = messageObj["content"].toString();
             }
         }
     }
     if (realContent.isEmpty())
        return true;

    mctx->content->append(realContent);
    mctx->parser->feed(realContent);

    for (auto action : mctx->parser->getResults()) {
        if (action.first == ToolParser::Text) {
            if (!action.second.isEmpty())
                this->textChainContent(action.second);
        } else if (action.first == ToolParser::Function) {
            QString func_content = action.second;
            if (!func_content.isEmpty()) {
                QString toolName;
                QJsonObject args;
                if (ToolParser::toJson(func_content, toolName, args)) {
                    if (m_toolList.contains(toolName)) {
                        qCInfo(logAgent) << "Agent" << name() << "calling tool:" << func_content << QThread::currentThreadId();

                        ToolUse tool;
                        tool.content = ToolParser::restoreFunction(func_content);
                        tool.name = toolName;
                        tool.params = QString::fromUtf8(QJsonDocument(args).toJson());
                        tool.index = m_usedTool.size();
                        this->toolUseContent(tool);

                        auto result = callTool(toolName, args);
                        tool.result = result.second;
                        bool isAbort = (result.first != AIServer::ErrorType::NoError)
                                     && (result.first != AIServer::ErrorType::MCPToolError);
                        tool.status = !result.first ? ToolUse::Completed : ToolUse::Failed;
                        m_usedTool.append(tool);
                        this->toolUseContent(tool);

                        if (!isAbort) {
                            if (result.first) {
                                qCWarning(logAgent) << "The tool" << toolName
                                                    << " was successfully called, but the tool execution was incorrect"
                                                    << result.second;
                            } else {
                                qCInfo(logAgent) << "Tool" << toolName << "executed successfully";
                            }

                            mctx->toolResults->append(QString("<tool_output>%0</tool_output>\n").arg(result.second));
                        } else {
                            qCWarning(logAgent) << "Tool execution failed for" << toolName << result.second;
                            return false;
                        }
                    } else {
                        qCWarning(logAgent) << "Invalid tool requested:" << toolName;
                    }
                } else {
                    qCWarning(logAgent) << "Failed to parse function content:" << func_content;
                }
            }
        }
    }

    return true;
}

QPair<int, QString> MCPChatAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    auto result = syncCall<QPair<int, QString>>([this, toolName, params]() {
        qCDebug(logAgent) << "call tool in" << QThread::currentThreadId();
        return m_mcpClient->callTool(m_name, toolName, params);
    });

    return result;
}

} // namespace uos_ai 
