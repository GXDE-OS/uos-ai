#include "text2imagehandler.h"
#include "generateprompt.h"
#include "conversation.h"

#include <QJsonDocument>

QString Text2ImageHandler::imagePrompt(const QJsonObject &response, const QString &conversation)
{
    QJsonObject arguments;
    const QJsonObject &tools = response.value("tools").toObject();
    if (tools.contains("function_call")) {
        const QJsonObject &fun = tools.value("function_call").toObject();
        arguments = QJsonDocument::fromJson(fun.value("arguments").toString().toUtf8()).object();
    }

    if (tools.contains("tool_calls")) {
        const QJsonArray &tool_calls = tools.value("tool_calls").toArray();

        for (const QJsonValue &tool_call : tool_calls) {
            const QJsonObject &fun = tool_call["function"].toObject();
            if (fun["name"].toString().endsWith("ImageGeneration")) {
                arguments = QJsonDocument::fromJson(fun.value("arguments").toString().toUtf8()).object();
                break;
            }
        }
    }

    QString prompt = arguments.value("desc").toString();

    if (prompt.isEmpty()) {
        prompt = Conversation::conversationLastUserData(conversation);
    }

    QString output = GeneratePrompt::Translate2EnPrompt(prompt);
    return output;
}
