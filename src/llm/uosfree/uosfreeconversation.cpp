// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "uosfreeconversation.h"

#include <QJsonDocument>
#include <QRegularExpressionMatchIterator>
#include <QDebug>

UOSAI_USE_NAMESPACE

UosFreeConversation::UosFreeConversation()
{

}

QJsonObject UosFreeConversation::parseContentString(const QString &content)
{
    QString result;
    QString reasoningContent;
    QMap<int, QJsonObject> toolCallMaps;

    QRegularExpression regex(R"(data:\s*\{(.*)\})");
    QRegularExpressionMatchIterator iter = regex.globalMatch(content);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        QString matchString = match.captured(0);

        int startIndex = matchString.indexOf('{');
        int endIndex = matchString.lastIndexOf('}');

        if (startIndex >= 0 && endIndex > startIndex) {
            QString matchContent = matchString.mid(startIndex, endIndex - startIndex + 1);
            QJsonObject rootObject = QJsonDocument::fromJson(matchContent.toUtf8()).object();
            if (!rootObject.contains("choices"))
                continue;
            QJsonArray choicesArray = rootObject["choices"].toArray();
            if (choicesArray.isEmpty())
                continue;
            QJsonObject choiceObject = choicesArray.at(0).toObject();
            if (!choiceObject.contains("delta"))
                continue;
            QJsonObject deltaObject = choiceObject["delta"].toObject();

            if (deltaObject.contains("content")) {
                result += deltaObject.value("content").toString();
            }
            if (deltaObject.contains("reasoning_content")) {
                reasoningContent += deltaObject.value("reasoning_content").toString();
            }
            if (deltaObject.contains("tool_calls")) {
                const QJsonArray &tool_calls = deltaObject.value("tool_calls").toArray();
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
                        const QJsonObject &tmpToolFun = toolCallObj.value("function").toObject();
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

    QJsonObject response;
    if (!result.isEmpty()) {
        response["content"] = result;
    }

    if (!reasoningContent.isEmpty()) {
        response["reasoningContent"] = reasoningContent;
    }

    QJsonObject tools;
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

void UosFreeConversation::update(const QByteArray &response)
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
            { "role",    "tools"                    },
            { "content", delateData.value("tools")  }
        }));
    }
}
