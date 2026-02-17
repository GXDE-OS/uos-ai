#ifndef DEEPSEEKCONVERSATION_H
#define DEEPSEEKCONVERSATION_H

#include "uosai_global.h"
#include "conversation.h"

namespace uos_ai {
class DeepSeekConversation : public Conversation
{
public:
    DeepSeekConversation();

public:
    void update(const QByteArray &response);

    static QJsonObject parseContentString(const QString &content);
};
}
#endif // DEEPSEEKCONVERSATION_H
