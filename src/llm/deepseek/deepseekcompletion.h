#ifndef DEEPSEEKCOMPLETION_H
#define DEEPSEEKCOMPLETION_H

#include "uosai_global.h"
#include "ainetwork.h"
#include "deepseekconversation.h"

namespace uos_ai {

class DeepSeekCompletion : public BaseNetWork
{
public:
    DeepSeekCompletion(const QString &url, const AccountProxy &account);

    QPair<int, QString> create(const QString &model, DeepSeekConversation &conversation, const QVariantHash &params);

private:
    QString rootUrl;
    QJsonArray transformFunctionList(const QJsonArray &inputArray);
};
}
#endif // DEEPSEEKCOMPLETION_H
