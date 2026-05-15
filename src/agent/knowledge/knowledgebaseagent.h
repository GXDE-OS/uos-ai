#ifndef KNOWLEDGEBASEAGENT_H
#define KNOWLEDGEBASEAGENT_H

#include "agent/llmagent.h"

#include <QObject>
#include <QString>

namespace uos_ai {

class KnowledgeBaseAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit KnowledgeBaseAgent(QObject *parent = nullptr);
    ~KnowledgeBaseAgent() override;

    /**
     * 在调用 LLM 前先进行知识库检索，将检索结果注入 system prompt
     */
    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &agentParams = {}) override;

protected:
    QString systemPrompt() const override;

private:
    /**
     * 多轮对话 Query 重写
     * 当存在历史记录时，用 LLM 生成一个自包含的检索 query
     * @param currentQuery 当前用户问题
     * @param history 对话历史
     * @return 重写后的查询，失败时返回原始 query
     */
    QString rewriteQuery(const QString &currentQuery, const QList<ModelMessage> &history);

    QString m_retrievedContext;

    static constexpr int kKnowledgeBaseTopK = 5;
    static constexpr int kMaxHistoryForRewrite = 4; // Query 重写时使用的最近历史条数
};

} // namespace uos_ai

#endif // KNOWLEDGEBASEAGENT_H
