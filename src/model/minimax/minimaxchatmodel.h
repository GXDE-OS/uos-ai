#ifndef MINIMAXCHATMODEL_H
#define MINIMAXCHATMODEL_H

#include "openai/oaichatmodel.h"

namespace uos_ai {

class MiniMaxChatModel : public OaiChatModel
{
    Q_OBJECT
public:
    explicit MiniMaxChatModel(QObject *parent = nullptr);
    ~MiniMaxChatModel() override;
    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;
};

} // namespace uos_ai

#endif // MINIMAXCHATMODEL_H
