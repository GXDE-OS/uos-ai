// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "geminiconversation.h"

#include <QJsonDocument>
#include <QDebug>
#include <QRegularExpressionMatchIterator>

UOSAI_USE_NAMESPACE

// https://github.com/google-gemini/cookbook/blob/main/quickstarts/rest/System_instructions_REST.ipynb
// https://aistudio.google.com/app/apikey
GeminiConversation::GeminiConversation()
{

}

bool GeminiConversation::setChatContent(const QString &data)
{
    if (!data.isEmpty()) {
        const QJsonDocument &document = QJsonDocument::fromJson(data.toUtf8());
        if (document.isArray()) {
            for (QJsonValue jv : document.array()) {
                QJsonObject obj = jv.toObject();
                QString str = obj.value("content").toString();
                if (obj.value("role") == "user") {
                    QJsonObject user;
                    user.insert("role", "user");
                    QJsonArray parts;
                    {
                        QJsonObject text;
                        text.insert("text", str);
                        parts.append(text);
                    }

                    user.insert("parts", parts);

                    m_contents.push_back(user);
                } else if (obj.value("role") == "assistant"){
                    QJsonObject model;
                    model.insert("role", "model");
                    QJsonArray parts;
                    {
                        QJsonObject text;
                        text.insert("text", str);
                        parts.append(text);
                    }
                    model.insert("parts", parts);
                    m_contents.push_back(model);
                }
            }
        } else {
            QJsonObject user;
            user.insert("role", "user");
            QJsonArray parts;
            {
                QJsonObject text;
                text.insert("text", data);
                parts.append(text);
            }
            user.insert("parts", parts);

            m_contents.push_back(user);
        }
        return true;
    }
    return false;
}

bool GeminiConversation::setSystemData(const QString &data)
{
    if (data.isEmpty()) {
        m_system = QJsonObject();
    } else {
        QJsonObject parts;
        QJsonObject text;
        text.insert("text", data);
        parts.insert("parts", text);
        m_system = parts;
    }

    return true;
}

bool GeminiConversation::setFunctions(const QJsonArray &functions)
{
    return false;
}

QJsonArray GeminiConversation::getChatContent() const
{
    return m_contents;
}

QJsonObject GeminiConversation::getSystemData() const
{
    return m_system;
}

QString GeminiConversation::getLastResponse() const
{
    if (!m_contents.isEmpty() && m_contents.last()["role"].toString() == "model") {
        QJsonArray partsArray =  m_contents.last()["parts"].toArray();
        for (const QJsonValue &partValue : partsArray) {
            QJsonObject partObject = partValue.toObject();
            if (!partObject.contains("text"))
                continue;

            QString text = partObject["text"].toString();
            return text;
        }
    }
    return QString();
}

void GeminiConversation::update(const QByteArray &response)
{
    if (response.isEmpty())
        return;

    if (response.startsWith("data:")) {
        const QJsonObject &delateData = parseContentString(response);
        if (delateData.contains("content")) {
            QJsonObject model;
            model.insert("role", "model");
            QJsonArray parts;
            {
                QJsonObject text;
                text.insert("text", delateData.value("content").toString());
                parts.append(text);
            }
            model.insert("parts", parts);
            m_contents.push_back(model);
        }
    }
}

QJsonObject GeminiConversation::parseContentString(const QString &content)
{
    QString candidatesContent;

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
            if (rootObject.contains("candidates")) {
                QJsonArray candidatesArray = rootObject["candidates"].toArray();

                // 遍历 candidates 数组
                for (const QJsonValue &candidateValue : candidatesArray) {
                    QJsonObject candidateObject = candidateValue.toObject();
                    if (!candidateObject.contains("content"))
                        continue;

                    QJsonObject contentObject = candidateObject["content"].toObject();
                    if (!contentObject.contains("parts"))
                        continue;

                    QJsonArray partsArray = contentObject["parts"].toArray();
                    // 遍历 parts 数组
                    for (const QJsonValue &partValue : partsArray) {
                        QJsonObject partObject = partValue.toObject();
                        if (!partObject.contains("text"))
                            continue;

                        QString text = partObject["text"].toString();
                        if (!text.isEmpty())
                            candidatesContent += text;
                    }
                }
            }
        }
    }

    QJsonObject response;
    if (!candidatesContent.isEmpty()) {
        response["content"] = candidatesContent;
    }

    return response;
}
