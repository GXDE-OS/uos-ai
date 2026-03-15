#ifndef RESEARCHAGENT_H
#define RESEARCHAGENT_H

#include "multiagent/sequentialagent.h"

#include <QMap>
#include <QSharedPointer>

namespace uos_ai {

class ResearchAgent : public SequentialAgent
{
    Q_OBJECT
public:
    explicit ResearchAgent(QObject *parent = nullptr);

    static QSharedPointer<LlmAgent> create();

    bool initialize() override;

    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params = {}) override;

protected:
    bool beforeSubAgentCall(const QString &agentName, QJsonObject &currentQuestion,
                                        QJsonArray &localMessages, const QJsonArray &globalMessages) override;
};

} // namespace uos_ai

#endif // RESEARCHAGENT_H
