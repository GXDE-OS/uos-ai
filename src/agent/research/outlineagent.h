#ifndef OUTLINEAGENT_H
#define OUTLINEAGENT_H

#include "llmagent.h"

namespace uos_ai {
class OutlineAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit OutlineAgent(QObject *parent = nullptr);
    virtual ~OutlineAgent();

    bool initialize() override;
    static QSharedPointer<LlmAgent> create();

    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &messages, const QVariantHash &params = {}) override;

private:
    void emitOutline(const QJsonObject &outline, const QString &articleId);
};

} // namespace uos_ai

#endif // OUTLINEAGENT_H
