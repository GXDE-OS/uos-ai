#ifndef XFCONVERSATION_H
#define XFCONVERSATION_H

#include "conversation.h"

class XFConversation : public Conversation
{
public:
    XFConversation();

public:
    bool addUserData(const QString &data);
    QPair<int, QString> update(const QByteArray &response);

public:
    static QPair<int, QJsonObject> parseContentString(const QString &content);
};

#endif // XFCONVERSATION_H
