#ifndef DEEPSEEKCHATMODEL_H
#define DEEPSEEKCHATMODEL_H

#include "openai/oaichatmodel.h"

namespace uos_ai {

class DeepSeekChatModel : public OaiChatModel
{
    Q_OBJECT
public:
    explicit DeepSeekChatModel(QObject *parent = nullptr);
    ~DeepSeekChatModel() override;
    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;
};

} // namespace uos_ai

#endif // DEEPSEEKCHATMODEL_H
