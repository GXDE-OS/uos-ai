#ifndef WXQFCHATCOMPLETION_H
#define WXQFCHATCOMPLETION_H

#include "wxqfnetwork.h"
#include "wxqfconversation.h"

class WXQFChatCompletion : public WXQFNetWork
{
public:
    explicit WXQFChatCompletion(const AccountProxy &account);

    QPair<int, QString> create(int model, const QString &url, WXQFConversation &conversation, const QVariantHash &params);
};

#endif // WXQFCHATCOMPLETION_H
