#include "conversation.h"

#include <QJsonDocument>
#include <QVariant>
#include <QRegularExpressionMatchIterator>

Conversation::Conversation()
{

}

Conversation::~Conversation()
{

}

QString Conversation::conversationLastUserData(const QString &conversation)
{
    const QJsonArray &array = QJsonDocument::fromJson(conversation.toUtf8()).array();
    if (!array.isEmpty() && array.last()["role"] == "user") {
        return array.last()["content"].toString();
    }

    return conversation;
}

bool Conversation::setSystemData(const QString &data)
{
    if (!data.isEmpty()) {
        for (auto iter = m_conversion.begin(); iter != m_conversion.end(); iter++) {
            if (iter->toObject().value("role").toString() == "system")
                return false;
        }
        m_conversion.insert(0, QJsonObject({ { "role", "system" }, {"content", data} }));
        return true;
    }
    return false;
}

bool Conversation::popSystemData()
{
    if (!m_conversion.isEmpty() && m_conversion.at(0)["role"].toString() == "system") {
        m_conversion.removeFirst();
        return true;
    }
    return false;
}

bool Conversation::addUserData(const QString &data)
{
    if (!data.isEmpty()) {
        const QJsonDocument &document = QJsonDocument::fromJson(data.toUtf8());
        if (document.isArray()) {
            m_conversion = document.array();
        } else {
            m_conversion.push_back(QJsonObject({ { "role", "user" }, {"content", data} }));
        }

        return true;
    }
    return false;
}

bool Conversation::popUserData()
{
    if (!m_conversion.isEmpty() && m_conversion.last()["role"].toString() == "user") {
        m_conversion.removeLast();
        return true;
    }
    return false;
}

QString Conversation::getLastResponse() const
{
    if (!m_conversion.isEmpty() && m_conversion.last()["role"].toString() == "assistant") {
        return m_conversion.last()["content"].toString();
    }
    return QString();
}

QByteArray Conversation::getLastByteResponse() const
{
    if (!m_conversion.isEmpty() && m_conversion.last()["role"].toString() == "assistant") {
        return m_conversion.last()["content"].toVariant().toByteArray();
    }
    return QByteArray();
}

bool Conversation::popLastResponse()
{
    if (!m_conversion.isEmpty() && m_conversion.last()["role"].toString() == "assistant") {
        m_conversion.removeLast();
        return true;
    }
    return false;
}

QJsonObject Conversation::getLastTools() const
{
    if (!m_conversion.isEmpty() && m_conversion.last()["role"].toString() == "tools") {
        return m_conversion.last()["content"].toObject();
    }

    return QJsonObject();
}

bool Conversation::popLastTools()
{
    if (!m_conversion.isEmpty() && m_conversion.last()["role"].toString() == "tools") {
        m_conversion.removeLast();
        return true;
    }
    return false;
}

bool Conversation::setFunctions(const QJsonArray &functions)
{
    m_functions = functions;
    return true;
}

QJsonArray Conversation::getConversions() const
{
    return m_conversion;
}

QJsonArray Conversation::getFunctions() const
{
    return m_functions;
}

QJsonArray Conversation::getFunctionTools() const
{
    QJsonArray tools;
    for (const QJsonValue &fun : m_functions) {
        QJsonObject tool;
        tool["type"] = "function";
        tool["function"] = fun;
        tools << tool;
    }

    return tools;
}

void Conversation::filterThink()
{
    for (int i = 0; i < m_conversion.size(); i++) {
        QJsonObject convObj = m_conversion.at(i).toObject();
        if (convObj.value("role").toString() == "assistant") {
            QString content = convObj.value("content").toString();
            QRegularExpression regex("<think>.*?</think>", QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatch match = regex.match(content);

            if (match.hasMatch()) {
                convObj.insert("content", content.replace(regex, ""));
                m_conversion.replace(i, convObj);
            }
        }
    }
}
