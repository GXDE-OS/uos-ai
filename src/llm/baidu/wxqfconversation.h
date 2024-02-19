#ifndef WXQFCONVERSATION_H
#define WXQFCONVERSATION_H

#include "conversation.h"

class WXQFConversation : public Conversation
{
public:
    WXQFConversation();

public:
    void update(const QByteArray &response);

public:
    QJsonObject parseContentString(const QByteArray &content);

private:
    QByteArray m_deltacontent;
};

#endif // WXQFCONVERSATION_H
