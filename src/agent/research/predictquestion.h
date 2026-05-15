#ifndef PREDICTQUESTION_H
#define PREDICTQUESTION_H

#include "llmagent.h"

namespace uos_ai {
class PredictQuestion : public LlmAgent
{
    Q_OBJECT
public:
    explicit PredictQuestion(QObject *parent = nullptr);

    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params = {}) override;

private:
    void emitGuessYouWant(const QStringList &questions);
};
}
#endif // PREDICTQUESTION_H
