#ifndef PERSONALKNOWLEDGEASSISTANT_H
#define PERSONALKNOWLEDGEASSISTANT_H

#include "abstractassistant.h"

namespace uos_ai {

class PersonalKnowledgeAssistant : public AbstractAssistant
{
    Q_OBJECT
public:
    explicit PersonalKnowledgeAssistant(QObject *parent = nullptr);
    ~PersonalKnowledgeAssistant() override;

    void cancel() override;
Q_SIGNALS:
    void requestCancel();
protected:
    void processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry);
protected:
    QVariantHash run() override;
};

} // namespace uos_ai

#endif // PERSONALKNOWLEDGEASSISTANT_H
