#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QJsonObject>
#include <QJsonArray>

class Conversation
{
public:
    Conversation();
    virtual ~Conversation();

    /**
     * @brief conversationLastUserData
     * @param conversation
     * @return
     */
    static QString conversationLastUserData(const QString &conversation);

public:
    bool setSystemData(const QString &data);
    bool popSystemData();

    bool addUserData(const QString &data);
    bool popUserData();

    QString getLastResponse() const;
    QByteArray getLastByteResponse() const;
    bool popLastResponse();

    QJsonObject getLastTools() const;
    bool popLastTools();

    bool setFunctions(const QJsonArray &functions);
    QJsonArray getFunctions() const;
    QJsonArray getFunctionTools() const;

    QJsonArray getConversions() const;

protected:
    QJsonArray m_conversion;
    QJsonArray m_functions;
};

#endif // CONVERSATION_H
