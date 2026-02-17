#ifndef XFCHATCOMPLETION_H
#define XFCHATCOMPLETION_H

#include "xfnetwork.h"

class XFChatCompletion : public XFNetWork
{
    Q_OBJECT
public:
    explicit XFChatCompletion(const AccountProxy &account, QObject *parent = nullptr);

    QPair<int, QString> create(int model, const QString &url, XFConversation &conversation, const QVariantHash &params);
};

#endif // XFCHATCOMPLETION_H
