#include "wxqfconversation.h"

#include <QJsonDocument>
#include <QRegularExpression>

WXQFConversation::WXQFConversation()
{

}

QJsonObject WXQFConversation::parseContentString(const QByteArray &content)
{
    m_deltacontent += content;
    if (!content.trimmed().endsWith("}")) {
        return QJsonObject();
    }

    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator iter = regex.globalMatch(m_deltacontent);

    QByteArray cacheDeltacontent;
    QMap<int, QString> seqContents;

    QJsonObject functionCall;
    QMap<int, QJsonObject> toolCallMaps;

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString matchString = match.captured(0);

        int startIndex = matchString.indexOf('{');
        int endIndex = matchString.lastIndexOf('}');

        if (startIndex >= 0 && endIndex > startIndex) {
            QString content = matchString.mid(startIndex, endIndex - startIndex + 1);

            QJsonObject j = QJsonDocument::fromJson(content.toUtf8()).object();
            if (j.isEmpty()) {
                cacheDeltacontent += matchString.toUtf8();
            } else {
                if (j.contains("result")) {
                    seqContents[j.value("sentence_id").toInt()] += j.value("result").toString();
                }

                if (j.contains("function_call")) {
                    const QJsonObject &function_call =  j.value("function_call").toObject();
                    if (function_call.contains("name")) {
                        functionCall["name"] = functionCall["name"].toString() + function_call.value("name").toString();
                    }

                    if (function_call.contains("arguments")) {
                        functionCall["arguments"] = functionCall["arguments"].toString() + function_call.value("arguments").toString();
                    }
                }

                if (j.contains("tool_calls")) {
                    const QJsonArray &tool_calls =  j.value("tool_calls").toArray();
                    for (const QJsonValue &tool_call : tool_calls) {
                        const QJsonObject &toolCallObj = tool_call.toObject();

                        int index = toolCallObj["index"].toInt();
                        if (!toolCallMaps[index].contains("function")) {
                            toolCallMaps[index]["function"] = QJsonObject();
                        }

                        toolCallMaps[index]["index"] = index;

                        if (toolCallObj.contains("id")) {
                            toolCallMaps[index]["id"] = toolCallObj.value("id");
                        }

                        if (toolCallObj.contains("type")) {
                            toolCallMaps[index]["type"] = toolCallObj.value("type");
                        }

                        if (toolCallObj.contains("function")) {
                            QJsonObject toolFun = toolCallMaps[index]["function"].toObject();
                            const QJsonObject &tmpToolFun =  toolCallObj.value("function").toObject();
                            if (tmpToolFun.contains("name")) {
                                toolFun["name"] = toolFun["name"].toString() + tmpToolFun.value("name").toString();
                            }

                            if (tmpToolFun.contains("arguments")) {
                                toolFun["arguments"] = toolFun["arguments"].toString() + tmpToolFun.value("arguments").toString();
                            }

                            toolCallMaps[index]["function"] = toolFun;
                        }
                    }
                }
            }
        }
    }

    m_deltacontent.clear();
    m_deltacontent += cacheDeltacontent;

    QString deltacontent;
    for (auto iter = seqContents.begin(); iter != seqContents.end(); iter++) {
        deltacontent += iter.value();
    }

    QJsonObject response;
    if (!deltacontent.isEmpty()) {
        response["content"] = deltacontent;
    }

    QJsonObject tools;
    if (!functionCall.isEmpty()) {
        tools["function_call"] = functionCall;
        response["tools"] = tools;
    }

    QJsonArray toolCalls;
    for (auto iter = toolCallMaps.begin(); iter != toolCallMaps.end(); iter++) {
        toolCalls << iter.value();
    }

    if (!toolCalls.isEmpty()) {
        tools["tool_calls"] = toolCalls;
        response["tools"] = tools;
    }

    return response;
}

void WXQFConversation::update(const QByteArray &response)
{
    if (response.isEmpty())
        return;

    const QJsonObject &delateData = parseContentString(response);
    if (delateData.contains("content")) {
        m_conversion.push_back(QJsonObject({
            { "role",    "assistant"    },
            { "content", delateData.value("content") }
        }));
    }

    if (delateData.contains("tools")) {
        m_conversion.push_back(QJsonObject({
            { "role",       "tools"    },
            { "content",    delateData.value("tools")    }
        }));
    }
}
