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

    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &messages, const QVariantHash &params = {}) override;

private:
    void OutlineContent(const QJsonObject &outline);
};

} // namespace uos_ai

#endif // OUTLINEAGENT_H
