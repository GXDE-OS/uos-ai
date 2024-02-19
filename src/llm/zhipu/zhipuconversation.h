#ifndef ZHIPUCONVERSATION_H
#define ZHIPUCONVERSATION_H

#include "conversation.h"

class ZhiPuConversation: public Conversation
{
public:
    ZhiPuConversation();

public:
    QPair<int, QString> update(const QByteArray &response);

public:
    static QPair<int, QString> parseContentString(const QString &content);
};

#endif // ZHIPUCONVERSATION_H
