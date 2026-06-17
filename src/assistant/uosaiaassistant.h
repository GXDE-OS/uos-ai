#ifndef UOSAIASSISTANT_H
#define UOSAIASSISTANT_H

#include "abstractassistant.h"
#include "modelinfo.h"

namespace uos_ai {
class UOSAIAssistant : public AbstractAssistant
{
    Q_OBJECT
public:
    explicit UOSAIAssistant(QObject *parent = nullptr);
    ~UOSAIAssistant() override;

    void cancel() override;
Q_SIGNALS:
    void requestCancel();
protected:
    void processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry);
    QString runOnlineSearch(ModelAccountPtr account, const ModelMessage &currentMessage, const QList<ModelMessage> &historyMsg);
    QVariantHash runGenericAgent(ModelAccountPtr account, const ModelMessage &currentMessage, const QList<ModelMessage> &historyMsg, const QString &searchContext);
protected:
    QVariantHash run() override;
    bool canceled = false;
};

} // namespace uos_ai

#endif // UOSAIASSISTANT_H
