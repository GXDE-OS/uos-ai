#ifndef CHATASSISTANT_H
#define CHATASSISTANT_H

#include "abstractassistant.h"

namespace uos_ai {

class ChatAssistant : public AbstractAssistant
{
    Q_OBJECT
public:
    ChatAssistant(QObject *parent = nullptr);
    void cancel() override;
Q_SIGNALS:
    void requestCancel();
protected:
    QVariantHash run() override;
};

}

#endif // CHATASSISTANT_H
