#ifndef WRITINGMASTERAGENT_H
#define WRITINGMASTERAGENT_H

#include "llmagent.h"
#include "writingstate.h"

#include <QSharedPointer>

namespace uos_ai {

class OutlineAgent;
class ResearchAgent;
class ArticleAdjustAgent;

/**
 * WritingMasterAgent: AI 写作流程的统一编排入口
 *
 * 阶段一（规则路由）：基于 WritingState.stage 直接分发，零 LLM 开销。
 *
 * 阶段二（LLM 意图分类）：
 *   对存在先验写作上下文（已有大纲或文章）的请求，先做一次轻量非流式
 *   LLM 分类调用，输出 {"intent": "outline"|"research"|"adjust"}，
 *   再按意图分发。支持自然语言驱动阶段跳转，例如：
 *     - "大纲不错，开始写吧"  → 确认大纲，发起深度研究（research）
 *     - "重新生成大纲"        → 回退到大纲阶段（outline），即使文章已存在
 *     - "第三节改得更专业"    → 文章修改（adjust）
 *   分类失败时自动降级回阶段一规则路由。
 *
 * 子智能体：
 *   OutlineAgent       → outline 意图
 *   ResearchAgent      → research 意图（SequentialAgent: DeepResearch + ReportWriter）
 *   ArticleAdjustAgent → adjust 意图（含修改与自由对话）
 */
class WritingMasterAgent : public LlmAgent
{
    Q_OBJECT
public:
    explicit WritingMasterAgent(QObject *parent = nullptr);

    static QSharedPointer<LlmAgent> create();

    bool initialize() override;

    /**
     * 将 LLM 分发给所有子智能体（每个子智能体独立 LLM 实例）。
     * m_llm 自身用于意图分类。
     */
    void setModel(QSharedPointer<LLM> llm) override;

    /**
     * 主入口：先分类意图，再分发到对应子智能体。
     */
    QJsonObject processRequest(const QJsonObject &question,
                               const QJsonArray &history,
                               const QVariantHash &params = {}) override;

    void cancel() override;

private:
    /**
     * LLM 意图分类（非流式、单次调用）。
     *
     * 优化：若当前无任何写作先验上下文（无大纲且无文章），
     * 直接返回 GenerateOutline，跳过 LLM 调用。
     *
     * @param question   当前用户消息
     * @param history    对话历史（用于补充上下文摘要）
     * @param state      当前写作状态
     * @return           分类出的目标阶段；分类失败时返回 state.stage()（降级）
     */
    WritingState::Stage classifyIntent(const QJsonObject &question,
                                       const QJsonArray &history,
                                       const WritingState &state);

    /** 根据阶段返回对应子智能体原始指针（不转移所有权）。Chat 阶段不经此函数。 */
    LlmAgent *subAgentForStage(WritingState::Stage stage) const;

    /**
     * 普通对话/解释性问答处理：直接用 m_llm 发起一次流式 LLM 请求，
     * 不经过任何写作子智能体。
     */
    QJsonObject handleChat(const QJsonObject &question,
                           const QJsonArray &history,
                           const QVariantHash &params);

    QSharedPointer<OutlineAgent>       m_outlineAgent;
    QSharedPointer<ResearchAgent>      m_researchAgent;
    QSharedPointer<ArticleAdjustAgent> m_adjustAgent;

    LlmAgent *m_activeAgent = nullptr; // 当前执行中的子智能体，用于 cancel 转发
};

} // namespace uos_ai

#endif // WRITINGMASTERAGENT_H
