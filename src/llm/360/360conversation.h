#ifndef CONVERSATION360_H
#define CONVERSATION360_H

#include "conversation.h"

class Conversation360 : public Conversation
{
public:
    Conversation360();

public:
    void update(const QByteArray &response);

public:
    static QString parseContentString(const QString &content);
};

#endif // CONVERSATION360_H
