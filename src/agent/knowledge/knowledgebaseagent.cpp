#include "knowledgebaseagent.h"
#include "agentstep.h"
#include "dbus/embeddingserver.h"
#include "global_define.h"
#include "global_key_define.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

KnowledgeBaseAgent::KnowledgeBaseAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name = "knowledge_base_agent";
    m_description = "Personal Knowledge Base Agent";
    m_systemPrompt = R"(# Role: Personal Knowledge Base Assistant

**Objective**: You are an intelligent assistant that answers questions based on the user's personal knowledge base. Your task is to provide accurate, relevant answers by using the retrieved information from the user's documents.

---

### OPERATIONAL PROTOCOLS

1. **Use Retrieved Information**: When answering questions, prioritize using the retrieved information from the knowledge base that is provided in the context.
2. **Answer Generation**: Based on the retrieved information, provide clear, accurate answers. If the knowledge base doesn't contain relevant information, inform the user politely.
3. **Language Consistency**: Respond in %1.

---

**CONSTRAINTS**
1. Always use the retrieved information provided in the context before generating answers.
2. If no relevant information is found, clearly state that the answer is not available in the knowledge base.
3. Be honest about the limitations of the available information.
4. Maintain a professional and helpful tone.)";

    QString language = QLocale::system().language() == QLocale::Chinese ? "Chinese" : "English";
    m_systemPrompt = m_systemPrompt.arg(language);
}

KnowledgeBaseAgent::~KnowledgeBaseAgent()
{
}

QString KnowledgeBaseAgent::systemPrompt() const
{
    if (m_retrievedContext.isEmpty())
        return m_systemPrompt;

    return m_systemPrompt + "\n\n---\n\n## Retrieved Knowledge\n\n" + m_retrievedContext;
}

QString KnowledgeBaseAgent::rewriteQuery(const QString &currentQuery, const QList<ModelMessage> &history)
{
    if (!m_llm || history.isEmpty())
        return currentQuery;

    // 构建历史摘要
    QStringList historyTexts;
    int startIdx = qMax(0, history.size() - kMaxHistoryForRewrite);
    for (int i = startIdx; i < history.size(); ++i) {
        const ModelMessage &msg = history[i];
        QString role = (msg.role == STR_KEY_USER) ? "User" : "Assistant";
        QString text;
        for (const auto &metaMsg : msg.content) {
            if (metaMsg.type == CntText) {
                text = metaMsg.data.toString();
                break;
            }
        }
        if (!text.isEmpty())
            historyTexts.append(QString("%1: %2").arg(role, text));
    }

    if (historyTexts.isEmpty())
        return currentQuery;

    // 构建重写 prompt
    QString rewritePrompt = QString(R"(You are a query rewriter for a knowledge base search system.

Given the conversation history and the current question, rewrite the current question into a standalone, self-contained query that captures the user's intent.

Rules:
1. The rewritten query should be self-contained and understandable without context
2. Keep the original meaning and intent
3. Do not add information not present in the conversation
4. Output ONLY the rewritten query, nothing else

Conversation history:
%1

Current question: %2

Rewritten query:)").arg(historyTexts.join("\n"), currentQuery);

    // 调用 LLM 进行重写（非流式）
    ModelMessage rewriteMsg;
    rewriteMsg.role = STR_KEY_USER;
    rewriteMsg.content.append({ContentType::CntText, rewritePrompt});

    QVariantHash params;
    params[STR_KEY_STREAM] = false;
    params[STR_KEY_MAX_TOKENS] = 256;

    QVariantHash output = m_llm->chatCompletion({rewriteMsg}, params);

    if (!m_llm->lastError().isEmpty()) {
        qCWarning(logAssistant) << "Query rewrite failed:" << m_llm->lastError().value(STR_KEY_MESSAGE).toString();
        return currentQuery;
    }

    // 提取重写后的 query
    QString rewrittenQuery;
    MetaMessageList contentList = MetaMessage::fromHash(output);
    for (const auto &metaMsg : contentList) {
        if (metaMsg.type == CntText) {
            rewrittenQuery = metaMsg.data.toString().trimmed();
            break;
        }
    }

    if (rewrittenQuery.isEmpty()) {
        qCWarning(logAssistant) << "Query rewrite returned empty result";
        return currentQuery;
    }

    qCDebug(logAssistant) << "Query rewritten:" << currentQuery << "->" << rewrittenQuery;
    return rewrittenQuery;
}

QVariantHash KnowledgeBaseAgent::processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &agentParams)
{
    // 从用户问题中提取文本内容
    QString userQuery;
    for (const auto &metaMsg : question.content) {
        if (metaMsg.type == CntText) {
            userQuery = metaMsg.data.toString();
            break;
        }
    }

    if (userQuery.isEmpty()) {
        qCWarning(logAssistant) << "Empty user query";
        QVariantHash result;
        result[STR_KEY_ERROR] = GErrorType::KnowledgeBaseEmptyQuery;
        result[STR_KEY_MESSAGE] = "Empty user query";
        return result;
    }

    // 多轮对话 Query 重写
    QString searchQuery = rewriteQuery(userQuery, history);

    // 搜索中 step
    emit messageReceived(makeAgentStep(tr("Searching knowledge base"), NsRunning));

    QElapsedTimer timer;
    timer.start();
    QString resultData = EmbeddingServer::getInstance().embeddingSearch(searchQuery, kKnowledgeBaseTopK);
    qint64 elapsedMs = timer.elapsed();

    QJsonArray results = QJsonDocument::fromJson(resultData.toUtf8()).object().value(STR_KEY_RESULT).toArray();

    QStringList contextParts;
    QStringList refLines;
    QSet<QString> seenDocPaths;
    int sourceIndex = 1;

    for (const QJsonValueRef res : results) {
        QJsonObject resObj = res.toObject();

        QString content = resObj.value(STR_KEY_CONTENT).toString();
        if (content.isEmpty())
            continue;

        QString docPath = resObj.value(STR_KEY_SOURCE).toString();
        QString docName = QFileInfo(docPath).fileName();
        contextParts.append(QString("[Source: %1]\n%2").arg(docName, content));

        // 每个文档只在引用列表中出现一次
        if (!seenDocPaths.contains(docPath)) {
            seenDocPaths.insert(docPath);
            QUrl fileUrl = QUrl::fromLocalFile(docPath);
            refLines.append(QString("%1. [%2](%3)").arg(sourceIndex++).arg(docName, fileUrl.toString(QUrl::EncodeSpaces)));
        }
    }

    // 搜索完成 step，标题带耗时（保留一位小数）
    QString timeStr;
    if (elapsedMs < 60000) {
        timeStr = tr("Search complete · %1s").arg(elapsedMs / 1000.0, 0, 'f', 1);
    } else {
        int mins = static_cast<int>(elapsedMs / 60000);
        double secs = (elapsedMs % 60000) / 1000.0;
        timeStr = tr("Search complete · %1m%2s").arg(mins).arg(secs, 0, 'f', 1);
    }
    emit messageReceived(makeAgentStep(timeStr, NsCompleted));

    // 将检索结果注入 system prompt，原始用户消息保持不变
    m_retrievedContext = contextParts.join("\n\n");
    QVariantHash response = LlmAgent::processRequest(question, history, agentParams);
    m_retrievedContext.clear();

    // 生成完成后追加引用列表（markdown 超链接，由 MarkdownRenderer 渲染）
    if (!refLines.isEmpty()) {
        QString refText = "\n\n**" + tr("References") + "**\n\n" + refLines.join("\n");
        RenderMessage textMsg;
        textMsg.type = CntText;
        textMsg.data = refText;
        emit messageReceived(RenderMessageList{textMsg});
    }

    return response;
}
