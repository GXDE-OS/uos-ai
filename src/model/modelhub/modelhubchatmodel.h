#ifndef MODELHUBCHATMODEL_H
#define MODELHUBCHATMODEL_H

#include "openai/oaichatmodel.h"
#include "externalllm/modelhubwrapper.h"

namespace uos_ai {

class ModelHubChatModel : public OaiChatModel
{
    Q_OBJECT
public:
    explicit ModelHubChatModel(QSharedPointer<ModelhubWrapper> ins, QObject *parent = nullptr);
    ~ModelHubChatModel() override;
    inline void setEnableTool(bool enable) {
        m_toolUse = enable;
    }

    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;
protected:
    QSharedPointer<ModelhubWrapper> wrapper;
    bool m_toolUse = false;
};

} // namespace uos_ai

#endif // MODELHUBCHATMODEL_H
