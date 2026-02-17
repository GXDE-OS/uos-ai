// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cozeagent.h"

#include "cozechatcompletion.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logLLM)
UOSAI_USE_NAMESPACE

CozeAgent::CozeAgent(const LLMServerProxy &serverproxy) : LLM(serverproxy)
{

}

QPair<int, QString> CozeAgent::verify()
{
    return {0, ""};
}

QJsonObject CozeAgent::predict(const QString &content, const QJsonArray &functions)
{
    Q_UNUSED(functions)
    qCInfo(logLLM) << "Starting Coze prediction with content length:" << content.length();

    chatFailed.first = 0;
    chatFailed.second.clear();

    CozeChatCompletion chatCompletion(m_accountProxy);
    connect(this, &CozeAgent::aborted, &chatCompletion, &CozeChatCompletion::requestAborted);
    connect(&chatCompletion, &CozeChatCompletion::readyReadDeltaContent, this, &CozeAgent::onReadyReadChatDeltaContent);

    QPair<int, QString> errorpair = chatCompletion.create(conversation(content));
    if (errorpair.first == 0 && chatFailed.first != 0)
        errorpair = chatFailed;

    setLastError(errorpair.first);
    setLastErrorString(errorpair.second);

    if (errorpair.first != 0)
        qCWarning(logLLM) << "Coze prediction failed with error:" << errorpair.first << "-" << errorpair.second;

    QJsonObject response;
    response["content"] = anwserContent;

    return response;
}

QJsonArray CozeAgent::conversation(const QString &content)
{
    QJsonArray ret;
    const QJsonDocument &document = QJsonDocument::fromJson(content.toUtf8());
    if (document.isArray()) {
        for (const QJsonValue &val : document.array()) {
            auto obj = val.toObject();
            obj.insert("content_type", "text");
            ret.append(obj);
        }
    } else {
        ret.push_back(QJsonObject({ { "role", "user" }, {"content", content}, {"content_type", "text"} }));
    }
    return ret;
}

QString CozeAgent::parseContentString(const QByteArray &content)
{
    m_deltacontent += content;
    if (!content.trimmed().endsWith("}")) {
        return "";
    }

    QString tmpText;
    {
        QRegularExpression regex(R"(event:\s*conversation\.message\.delta\ndata:\s*\{(.*)\})");
        QRegularExpressionMatchIterator iter = regex.globalMatch(m_deltacontent);
        while (iter.hasNext()) {
            QRegularExpressionMatch match = iter.next();
            QString matchString = match.captured(0);

            int startIndex = matchString.indexOf('{');
            int endIndex = matchString.lastIndexOf('}');
            if (startIndex >= 0 && endIndex > startIndex) {
                QString content = matchString.mid(startIndex, endIndex - startIndex + 1);
                QJsonObject j = QJsonDocument::fromJson(content.toUtf8()).object();
                if (j.contains("role") && j.contains("content")) {
                    QString role = j.value("role").toString();
                    QString text = j.value("content").toString();
                    QString type = j.value("type").toString();
                    if (role == "assistant" && type == "answer") {
                        tmpText.append(text);
                        anwserContent.append(text);
                    }
                }
            }
        }
    }

    // chat failed
    if (anwserContent.isEmpty()) {
        QRegularExpression regex(R"(event:\s*conversation\.chat\.failed\ndata:\s*\{(.*)\})");
        QRegularExpressionMatchIterator iter = regex.globalMatch(m_deltacontent);
        while (iter.hasNext()) {
            qCWarning(logLLM) << "Coze chat failed, response content:" << m_deltacontent;
            QRegularExpressionMatch match = iter.next();
            QString matchString = match.captured(0);

            int startIndex = matchString.indexOf('{');
            int endIndex = matchString.lastIndexOf('}');
            if (startIndex >= 0 && endIndex > startIndex) {
                QString scontent = matchString.mid(startIndex, endIndex - startIndex + 1);
                QJsonObject j = QJsonDocument::fromJson(scontent.toUtf8()).object();
                if (j.contains("last_error")) {
                    j = j.value("last_error").toObject();
                    if (j.contains("code") && j.contains("msg")) {
                        chatFailed.first = j.value("code").toInt();
                        chatFailed.second = j.value("msg").toString();
                    }
                }
            }
        }
    }

    m_deltacontent.clear();
    return tmpText;
}

void CozeAgent::onReadyReadChatDeltaContent(const QByteArray &content)
{
    QString cur = parseContentString(content);
    if (!cur.isEmpty()) {
        m_replied = true;
        textChainContent(cur);
    }
}
