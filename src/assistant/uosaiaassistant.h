#ifndef UOSAIASSISTANT_H
#define UOSAIASSISTANT_H

#include "abstractassistant.h"

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
protected:
    QVariantHash run() override;
};

} // namespace uos_ai

#endif // UOSAIASSISTANT_H
