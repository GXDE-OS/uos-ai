#include "writingmasteragent.h"
#include "outlineagent.h"
#include "researchagent.h"
#include "articleadjustagent.h"
#include "writingstate.h"
#include "articlemodel.h"
#include "tools/researchtools.h"
#include "wrapper/llmservicevendor.h"
#include "networkdefs.h"
#include "util.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {

// ---------------------------------------------------------------------------
// Construction & factory
// ---------------------------------------------------------------------------

WritingMasterAgent::WritingMasterAgent(QObject *parent)
    : LlmAgent(parent)
{
    m_name        = "WritingMasterAgent";
    m_description = "Orchestrates the full AI writing workflow: outline, research, article, and adjustment.";
}

QSharedPointer<LlmAgent> WritingMasterAgent::create()
{
    return QSharedPointer<LlmAgent>(new WritingMasterAgent());
}

// ---------------------------------------------------------------------------
// initialize / setModel
// ---------------------------------------------------------------------------

bool WritingMasterAgent::initialize()
{
    auto *outlineAgent = new OutlineAgent(this);
    outlineAgent->initialize();
    m_outlineAgent = QSharedPointer<OutlineAgent>(outlineAgent);
    connect(outlineAgent, &LlmAgent::readyReadChatDeltaContent,
            this, &WritingMasterAgent::readyReadChatDeltaContent);

    auto *researchAgent = new ResearchAgent(this);
    researchAgent->initialize();
    m_researchAgent = QSharedPointer<ResearchAgent>(researchAgent);
    connect(researchAgent, &LlmAgent::readyReadChatDeltaContent,
            this, &WritingMasterAgent::readyReadChatDeltaContent);

    auto *adjustAgent = new ArticleAdjustAgent(this);
    adjustAgent->initialize();
    m_adjustAgent = QSharedPointer<ArticleAdjustAgent>(adjustAgent);
    connect(adjustAgent, &LlmAgent::readyReadChatDeltaContent,
            this, &WritingMasterAgent::readyReadChatDeltaContent);

    return true;
}

void WritingMasterAgent::setModel(QSharedPointer<LLM> llm)
{
    LlmAgent::setModel(llm); // m_llm = llm，用于意图分类

    auto account = llm->account();

    auto setSubModel = [&](QSharedPointer<LlmAgent> agent) {
        if (!agent)
            return;
        auto subLlm = LLMVendor()->getCopilot(account);
        // 子智能体的取消信号链：主 LLM abort → 子 LLM abort
        connect(llm.data(), &LLM::aborted, subLlm.data(), &LLM::aborted, Qt::DirectConnection);
        agent->setModel(subLlm);
    };

    setSubModel(m_outlineAgent);
    setSubModel(m_researchAgent);
    setSubModel(m_adjustAgent);
}

// ---------------------------------------------------------------------------
// processRequest：意图分类 + 分发
// ---------------------------------------------------------------------------

QJsonObject WritingMasterAgent::processRequest(const QJsonObject &question,
                                               const QJsonArray &history,
                                               const QVariantHash &params)
{
    WritingState state = WritingState::fromParams(params);

    // 阶段二：LLM 意图分类
    // 若无先验写作上下文，classifyIntent 内部会直接跳过 LLM 调用（快速路径）
    WritingState::Stage targetStage = classifyIntent(question, history, state);

    qCInfo(logAgent) << "WritingMasterAgent:"
                     << "state.stage =" << static_cast<int>(state.stage())
                     << "-> classified stage =" << static_cast<int>(targetStage)
                     << "| hasOutline =" << state.hasOutline()
                     << "| hasArticle =" << state.hasArticle();

    if (targetStage == WritingState::Stage::Chat)
        return handleChat(question, history, params);

    LlmAgent *targetAgent = subAgentForStage(targetStage);
    if (!targetAgent) {
        qCWarning(logAgent) << "WritingMasterAgent: subAgentForStage returned nullptr";
        return QJsonObject();
    }

    m_activeAgent = targetAgent;
    QJsonObject result = targetAgent->processRequest(question, history, params);
    m_activeAgent = nullptr;

    if (targetAgent->lastError() != AIServer::NoError) {
        m_llm->setLastError(targetAgent->lastError());
        m_llm->setLastErrorString(targetAgent->lastErrorString());
    }

    return result;
}

void WritingMasterAgent::cancel()
{
    LlmAgent::cancel();
    if (m_activeAgent)
        m_activeAgent->cancel();
}

// ---------------------------------------------------------------------------
// classifyIntent：轻量 LLM 意图分类（非流式单次调用）
// ---------------------------------------------------------------------------

WritingState::Stage WritingMasterAgent::classifyIntent(const QJsonObject &question,
                                                       const QJsonArray &history,
                                                       const WritingState &state)
{
    // -----------------------------------------------------------------------
    // 快速路径：无先验写作上下文时无需分类，直接生成大纲
    // -----------------------------------------------------------------------
    if (!state.hasOutline() && !state.hasArticle()) {
        qCInfo(logAgent) << "WritingMasterAgent: no prior context, skip classification -> GenerateOutline";
        return WritingState::Stage::GenerateOutline;
    }

    if (!m_llm) {
        qCWarning(logAgent) << "WritingMasterAgent: m_llm is null, fallback to rule-based";
        return state.stage();
    }

    // -----------------------------------------------------------------------
    // 构建分类 Prompt
    // -----------------------------------------------------------------------
    QString userMessage = question.value("content").toString();

    // 当前写作状态描述
    QString stateDesc;
    QString contextHint;
    switch (state.stage()) {
    case WritingState::Stage::GenerateOutline:
        stateDesc   = "An outline is being created.";
        contextHint = QString();
        break;
    case WritingState::Stage::GenerateArticle: {
        stateDesc = "An outline has been confirmed but the article has not been written yet.";
        QString outlineMd;
        ResearchTools::outlineJson2Md(state.outline(), 1, outlineMd);
        if (!outlineMd.isEmpty())
            contextHint = QString("Current outline (summary):\n%1").arg(outlineMd.left(600));
        break;
    }
    case WritingState::Stage::AdjustArticle: {
        stateDesc = "The article has been written and is available for adjustment.";
        // 提供文章章节结构（带 ID）供 LLM 感知，但截断以控制 token 用量
        ArticleModel model = ArticleModel::fromMarkdown(state.articleContent());
        if (!model.isEmpty()) {
            QStringList sectionTitles;
            for (const auto &sec : model.sections())
                sectionTitles << QString("[ID: %1] %2").arg(sec.id, sec.title);
            contextHint = QString("Article sections:\n%1").arg(sectionTitles.join("\n"));
        }
        break;
    }
    }

    QString classifyPrompt = QString(
R"(You are a writing workflow router. Analyze the user's message and output a single JSON object.

Current writing state: %1
%2
User message: "%3"

Determine the routing intent:
- "outline"   : User wants to generate, modify, or entirely redo the document outline.
                 (e.g., "change section 2", "redo the outline", "修改大纲", "重新生成大纲")
- "research"  : User approves/confirms the outline and wants to START writing the full article.
                 (e.g., "start writing", "looks good proceed", "ok go ahead", "开始写", "不错继续", "确认大纲开始写")
- "adjust"    : User wants to modify existing article content.
                 (e.g., "rewrite section 3", "make it more professional", "修改第三节", "删除第二段")
- "chat"      : User is asking a question, requesting an explanation, or having a general conversation unrelated to directly triggering a writing step.
                 (e.g., "what does this mean?", "解释一下", "你觉得这个主题怎么样", "谢谢", "帮我分析一下大纲的逻辑")

Rules:
1. If the user says "start writing" or similar confirmation of the outline → "research".
2. If the user explicitly requests changes to the outline structure → "outline".
3. If the user explicitly requests changes to the article body → "adjust".
4. If the user is asking, explaining, chatting, or giving feedback without requesting a concrete writing action → "chat".
5. When uncertain, prefer "chat" over switching the writing stage.

Respond with ONLY valid JSON, no extra text. Example: {"intent": "chat"})"
    ).arg(stateDesc, contextHint, userMessage);

    // -----------------------------------------------------------------------
    // 非流式 LLM 分类调用
    // -----------------------------------------------------------------------
    QJsonArray classifyMessages;
    QJsonObject sysMsg;
    sysMsg["role"]    = "system";
    sysMsg["content"] = "You are a precise JSON routing classifier for a writing assistant workflow. Output only valid JSON.";
    classifyMessages.append(sysMsg);

    QJsonObject userMsg;
    userMsg["role"]    = "user";
    userMsg["content"] = classifyPrompt;
    classifyMessages.append(userMsg);

    m_llm->switchStream(false);
    QJsonObject output = m_llm->predict(QJsonDocument(classifyMessages).toJson(), QJsonArray());
    m_llm->switchStream(true);

    if (m_llm->lastError() != AIServer::NoError) {
        qCWarning(logAgent) << "WritingMasterAgent: classification LLM error:" << m_llm->lastErrorString()
                            << "fallback to rule-based stage =" << static_cast<int>(state.stage());
        // 清除错误，避免误传到上层
        m_llm->setLastError(AIServer::NoError);
        m_llm->setLastErrorString(QString());
        return state.stage();
    }

    // -----------------------------------------------------------------------
    // 解析 JSON 意图
    // -----------------------------------------------------------------------
    QString content = output.value("content").toString().trimmed();

    // 去除可能的 markdown code fence（```json ... ```）
    static QRegularExpression fenceRe(R"(```(?:json)?\s*([\s\S]*?)\s*```)",
                                      QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch fenceMatch = fenceRe.match(content);
    if (fenceMatch.hasMatch())
        content = fenceMatch.captured(1).trimmed();

    // 尝试从响应中提取第一个 JSON 对象
    int braceStart = content.indexOf('{');
    int braceEnd   = content.lastIndexOf('}');
    if (braceStart != -1 && braceEnd > braceStart)
        content = content.mid(braceStart, braceEnd - braceStart + 1);

    QJsonObject intentObj = QJsonDocument::fromJson(content.toUtf8()).object();
    QString intent = intentObj.value("intent").toString().toLower().trimmed();

    qCInfo(logAgent) << "WritingMasterAgent: classification result intent =" << intent
                     << "(user message:" << userMessage << ")";

    if (intent == "outline")   return WritingState::Stage::GenerateOutline;
    if (intent == "research")  return WritingState::Stage::GenerateArticle;
    if (intent == "adjust")    return WritingState::Stage::AdjustArticle;
    if (intent == "chat")      return WritingState::Stage::Chat;

    // 降级：无法识别意图时保持当前阶段
    qCWarning(logAgent) << "WritingMasterAgent: unrecognized intent '" << intent
                        << "', fallback to current stage =" << static_cast<int>(state.stage());
    return state.stage();
}

// ---------------------------------------------------------------------------
// subAgentForStage
// ---------------------------------------------------------------------------

LlmAgent *WritingMasterAgent::subAgentForStage(WritingState::Stage stage) const
{
    switch (stage) {
    case WritingState::Stage::GenerateOutline:  return m_outlineAgent.data();
    case WritingState::Stage::GenerateArticle:  return m_researchAgent.data();
    case WritingState::Stage::AdjustArticle:    return m_adjustAgent.data();
    default:                                    return m_outlineAgent.data();
    }
}

// ---------------------------------------------------------------------------
// handleChat：普通对话/解释性问答，直接发起一次流式 LLM 请求
// ---------------------------------------------------------------------------

QJsonObject WritingMasterAgent::handleChat(const QJsonObject &question,
                                           const QJsonArray &history,
                                           const QVariantHash &params)
{
    Q_UNUSED(params)

    // 临时替换系统提示和工具，确保此次请求不带写作工具
    QString savedPrompt = m_systemPrompt;
    QJsonArray savedTools  = m_tools;

    m_systemPrompt = "You are a helpful writing assistant. Answer the user's question clearly and concisely.";
    m_tools        = QJsonArray();

    QJsonObject result = LlmAgent::processRequest(question, history, params);

    m_systemPrompt = savedPrompt;
    m_tools        = savedTools;

    return result;
}

} // namespace uos_ai
