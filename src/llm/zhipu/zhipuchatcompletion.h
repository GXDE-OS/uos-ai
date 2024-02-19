#ifndef ZHIPUCHATCOMPLETION_H
#define ZHIPUCHATCOMPLETION_H

#include "zhipunetwork.h"
#include "zhipuconversation.h"

class ZhiPuChatCompletion : public ZhiPuNetWork
{
public:
    explicit ZhiPuChatCompletion(const AccountProxy &account);

    QPair<int, QString> create(int mode, ZhiPuConversation &conversation, qreal temperature = 1.0);
};

#endif // ZHIPUCHATCOMPLETION_H
