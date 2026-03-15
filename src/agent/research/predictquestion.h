#ifndef PREDICTQUESTION_H
#define PREDICTQUESTION_H

#include "agent/llmagent.h"

namespace uos_ai {
class PredictQuestion : public LlmAgent
{
    Q_OBJECT
public:
    explicit PredictQuestion(QObject *parent = nullptr);

    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &message, const QVariantHash &params = {}) override;

private:
    void predictContent(const QJsonArray &questions);
};
}
#endif // PREDICTQUESTION_H
