#ifndef UOSCLAW_H
#define UOSCLAW_H

#include "abstractassistant.h"

namespace uos_ai {

class UOSClaw : public AbstractAssistant
{
    Q_OBJECT
public:
    explicit UOSClaw(QObject *parent = nullptr);

    void cancel() override;
    static QString faq();
Q_SIGNALS:
    void requestCancel();
protected:
    void processMessage(ModelMessage &currentMessage, QList<ModelMessage> &historyMsg, bool retry);
protected:
    QVariantHash run() override;
};

}

#endif // UOSCLAW_H
