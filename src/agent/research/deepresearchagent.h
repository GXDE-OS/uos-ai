#ifndef DEEPRESEARCHAGENT_H
#define DEEPRESEARCHAGENT_H

#include "llmagent.h"
#include "referencemanager.h"

namespace uos_ai {

class DeepResearchAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit DeepResearchAgent(QObject *parent = nullptr);
    ~DeepResearchAgent() override;

    bool initialize() override;
    static QSharedPointer<LlmAgent> create();
    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params = {}) override;

protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

private:
    QJsonArray handleWebSearchTool(const QJsonObject &arg);
    QJsonArray webSearch(const QString &query);

    QJsonArray handleLocalSearchTool(const QJsonObject &arg);
    QJsonArray localSearch(const QString &query);

    void emitAction(const QString &name, NormalStatus status, const QString &params, const QString &result, int index = 0);

    ReferenceManager m_refManager;

    ModelTool m_webSearchTool;
    ModelTool m_localSearchTool;

    int m_searchFailCount = 0;
    static constexpr int kMaxSearchFails = 3;
};

} // namespace uos_ai

#endif // DEEPRESEARCHAGENT_H
