#ifndef BAILIANCHATMODEL_H
#define BAILIANCHATMODEL_H

#include "../openai/oaichatmodel.h"

namespace uos_ai {

class BailianChatModel : public OaiChatModel
{
    Q_OBJECT
public:
    explicit BailianChatModel(QObject *parent = nullptr);
    ~BailianChatModel() override;

    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;
};

}

#endif // BAILIANCHATMODEL_H
