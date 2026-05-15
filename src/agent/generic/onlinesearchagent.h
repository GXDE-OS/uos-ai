#ifndef ONLINESEARCHAGENT_H
#define ONLINESEARCHAGENT_H

#include "llmagent.h"

#include <QObject>

namespace uos_ai {

class OnlineSearchAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit OnlineSearchAgent(QObject *parent = nullptr);
    QString systemPrompt() const override;

signals:

public slots:
};

}

#endif // ONLINESEARCHAGENT_H
