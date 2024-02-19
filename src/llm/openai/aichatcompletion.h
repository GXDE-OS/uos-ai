#ifndef AICHATCOMPLETION_H
#define AICHATCOMPLETION_H

#include "ainetwork.h"
#include "aiconversation.h"

class AIChatCompletion : public AINetWork
{
public:
    explicit AIChatCompletion(const AccountProxy &account);

    QPair<int, QString> create(const QString &model, AIConversation &conversation, qreal temperature = 1.0);
};

#endif // AICHATCOMPLETION_H
