#include "xfconversation.h"
#include "xfcodetranslation.h"

#include <QJsonDocument>
#include <QMap>
#include <QRegularExpression>
#include <QDebug>

XFConversation::XFConversation()
{

}

bool XFConversation::addUserData(const QString &data)
{
    if (!data.isEmpty()) {
        const QJsonDocument &document = QJsonDocument::fromJson(data.toUtf8());
        if (document.isArray()) {
            QJsonArray array = document.array();
            // 讯飞不支持function请求，这里改成user
            if (array.last()["role"] == "function") {
                QJsonObject conversion = array.last().toObject();
                conversion["role"] = "user";
                array[array.count() - 1] = conversion;
            }
            m_conversion = array;
        } else {
            m_conversion.push_back(QJsonObject({ { "role", "user" }, {"content", data} }));
        }
        return true;
    }
    return false;
}

QPair<int, QJsonObject> XFConversation::parseContentString(const QString &content)
{
    QJsonObject functionCall;
    QMap<int, QJsonObject> toolCallMaps;
    QMap<int, QByteArray> seqContents;

    QStringList jsonStringList = content.split(QRegularExpression("(?<=\\})\\s*(?=\\{)"));
    for (const QString &jsonStr : jsonStringList) {
        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);

        if (error.error != QJsonParseError::NoError) {
            qWarning() << "XFConversation JSON parsing error:" << jsonStr;
            return QPair<int, QJsonObject>(0, QJsonObject());
        }

        if (!jsonDoc.isObject()) {
            qWarning() << "XFConversation JSON is not an object." << jsonStr;
            return QPair<int, QJsonObject>(0, QJsonObject());
        }

        QJsonObject jsonObject = jsonDoc.object();
        QJsonObject header = jsonObject["header"].toObject();
        int code = header["code"].toInt();
        if (code != 0) {
            qWarning() << "请求错误:" << code << "," << jsonObject;
            QString errorMessage = jsonStr;
            if (header.contains("message")) {
                errorMessage = XFCodeTranslation::serverCodeTranslation(code, header.value("message").toString());
            }

            QJsonObject errorObj;
            errorObj["content"] = errorMessage;
            return qMakePair(code, errorObj);
        } else if (jsonObject.contains("payload")) {
            QJsonObject choices = jsonObject.value("payload").toObject().value("choices").toObject();
            int seq = choices.value("seq").toInt();
            QJsonObject j = choices.value("text").toArray()[0].toObject();
            QByteArray content = j.value("content").toVariant().toByteArray();
            seqContents[seq] += content;

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

    QByteArray resultcontent;
    for (auto iter = seqContents.begin(); iter != seqContents.end(); iter++) {
        resultcontent += iter.value();
    }

    QJsonObject response;
    if (!resultcontent.isEmpty()) {
        response["content"] = QString(resultcontent);
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

    return qMakePair(0, response);
}

QPair<int, QString> XFConversation::update(const QByteArray &response)
{
    if (response.isEmpty())
        return QPair<int, QString>(0, QString());

    const QJsonObject &j = QJsonDocument::fromJson(response).object();
    if (j.contains("choices")) {
        const QJsonArray &texts = j["choices"].toObject().value("text").toArray();
        for (auto text = texts.begin(); text != texts.end(); text++) {
            const QJsonObject &tx = text->toObject();
            m_conversion.push_back(QJsonObject({
                { "role",    tx["role"]    },
                { "content", tx["content"] }
            }));
        }
    } else {
        const QPair<int, QJsonObject> &resultPair = parseContentString(response);
        if (resultPair.first == 0 && !resultPair.second.isEmpty()) {
            if (resultPair.second.contains("content")) {
                m_conversion.push_back(QJsonObject({
                    { "role",    "assistant"},
                    { "content", resultPair.second.value("content") }
                }));
            }

            if (resultPair.second.contains("tools")) {
                m_conversion.push_back(QJsonObject({
                    { "role",    "tools"    },
                    { "content", resultPair.second.value("tools")    }
                }));
            }

            return QPair<int, QString>(0, QString());
        }

        return qMakePair(resultPair.first, resultPair.second.value("content").toString());
    }

    return QPair<int, QString>(0, QString());
}
