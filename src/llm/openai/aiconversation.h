#ifndef AICONVERSATION_H
#define AICONVERSATION_H

#include "conversation.h"

class AIConversation : public Conversation
{
public:
    AIConversation();

public:
    void update(const QByteArray &response);

public:
    static QJsonObject parseContentString(const QString &content);
};

#endif // CONVERSATION_H
