#ifndef XFCHATCOMPLETION_H
#define XFCHATCOMPLETION_H

#include "xfnetwork.h"

class XFChatCompletion : public XFNetWork
{
    Q_OBJECT
public:
    explicit XFChatCompletion(const AccountProxy &account, QObject *parent = nullptr);

    QPair<int, QString> create(int model, XFConversation &conversation, qreal temperature = 1.0);
};

#endif // XFCHATCOMPLETION_H
