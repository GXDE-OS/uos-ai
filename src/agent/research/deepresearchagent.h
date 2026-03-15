#ifndef DEEPRESEARCHAGENT_H
#define DEEPRESEARCHAGENT_H

#include "llmagent.h"

namespace uos_ai {
class DeepAnalyzer;
class DeepResearchAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit DeepResearchAgent(QObject *parent = nullptr);
    virtual ~DeepResearchAgent() override;
    
    bool initialize() override;
    static QSharedPointer<LlmAgent> create();
    QJsonObject processRequest(const QJsonObject &question, const QJsonArray &history, const QVariantHash &params = {}) override;

    QJsonArray getReferences() const;

protected:
    QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;

private:
    QJsonArray handleWebSearchTool(const QJsonObject &arg);
    QJsonArray webSearch(const QString &query);

    QJsonArray handleLocalSearchTool(const QJsonObject &arg);
    QJsonArray localSearch(const QString &query);

    QJsonArray handleFileParseTool(const QJsonObject &arg);
    QJsonArray fileParse(const QStringList &files);

    QString handleDeepAnalyzer(const QJsonArray &results, const QString &initTask);
    QString deepAnalyze(const QJsonArray &searchContent, const QString &task);
    QString addReferencesAndFormat(const QJsonArray &results);
    void outlineJson2Md(const QJsonObject &outlineObj, int level, QString &markdown);
    QString findRefIdBySource(const QString &source) const;

    QString m_userTask;

    QJsonArray m_accumulatedReferences;
    int m_refOffset = 0;
};

} // namespace uos_ai

#endif // DEEPRESEARCHAGENT_H
