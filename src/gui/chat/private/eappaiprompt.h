#ifndef EAIEAIPROMPT_H
#define EAIEAIPROMPT_H

#include "eaiprompt.h"

//Add all app ai prompt in this file.

//对话模式
class EConversationPrompt : public EAiPrompt
{
public:
    explicit EConversationPrompt(const QString &userData);

    QString getAiPrompt() override;
};

#endif // EAIEAIPROMPT_H
