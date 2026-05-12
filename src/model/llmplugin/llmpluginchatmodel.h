#ifndef LLMPLUGINCHATMODEL_H
#define LLMPLUGINCHATMODEL_H

#include "abstractchatmodel.h"

namespace uos_ai {

class LLMModel;

class LLMPluginChatModel : public AbstractChatModel
{
    Q_OBJECT
public:
    explicit LLMPluginChatModel(QObject *parent = nullptr);
    ~LLMPluginChatModel() override;

    void setLLMModel(LLMModel *model);
    LLMModel* llmModel() const;

    QVariantHash chatCompletion(const QList<ModelMessage> &messages, const QVariantHash &modelParams = QVariantHash()) override;

public Q_SLOTS:
    void cancel() override;

protected:
    static bool streamCallback(const QString &deltaData, void *user);
    QString buildMessages(const QList<ModelMessage> &messages);
protected:
    LLMModel *m_llmModel = nullptr;
    bool m_abortFlag = false;
};

}

#endif
