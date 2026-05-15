#ifndef ONLINESEARCHMODEL_H
#define ONLINESEARCHMODEL_H

#include "openai/oaichatmodel.h"

namespace uos_ai {

class OnlineSearchModel : public OaiChatModel
{
    Q_OBJECT
public:
    explicit OnlineSearchModel(QObject *parent = nullptr);
    ~OnlineSearchModel() override;
    void setApiHost(const QString &host) override;
    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;

protected:
    inline QString searchThinkingId() {
        return QString("bot-20240827214959-w7mj7");
    }

    inline QString searchId() {
        return QString("bot-20250321110601-77w5l");
    }
};

} // namespace uos_ai

#endif // ONLINESEARCHMODEL_H
