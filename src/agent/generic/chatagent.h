#ifndef CHATAGENT_H
#define CHATAGENT_H

#include "llmagent.h"

#include <QString>
#include <QJsonObject>
#include <QPair>

namespace uos_ai {

class ChatAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit ChatAgent(QObject *parent = nullptr);
protected:
    QList<ModelMessage> initChatMessages(const ModelMessage &question, const QList<ModelMessage> &history) const override;
};

}

#endif // CHATAGENT_H
