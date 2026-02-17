#ifndef CHATCOMPLETION360_H
#define CHATCOMPLETION360_H

#include "360network.h"
#include "360conversation.h"

class ChatCompletion360 : public NetWork360
{
public:
    explicit ChatCompletion360(const AccountProxy &account);

    QPair<int, QString> create(const QString &model, Conversation360 &conversation, const QVariantHash &params);
};

#endif // CHATCOMPLETION360_H
