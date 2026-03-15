#ifndef WRITINGSTATE_H
#define WRITINGSTATE_H

#include <QString>
#include <QJsonObject>
#include <QVariantHash>

namespace uos_ai {

/**
 * WritingState: 写作项目的结构化状态快照
 *
 * 取代原有散落在 params 中的 outline(QVariantHash)、article(QString) 等碎片化数据，
 * 统一描述 AI 写作会话在某一时刻的阶段与关键数据。
 *
 * EAiExecutor 构建此对象并序列化放入 params["writing_state_json"]，
 * WritingMasterAgent 及各子 Agent 从 params 中反序列化读取。
 */
class WritingState
{
public:
    enum class Stage {
        GenerateOutline = 0,  // 初始阶段：尚无大纲，需要生成
        GenerateArticle = 1,  // 大纲已确认：执行深度研究并生成文章
        AdjustArticle   = 2,  // 文章已存在：处理修改
        Chat            = 3,  // 普通对话/解释性问答，不触发写作流程
    };

    WritingState() = default;

    // --- Stage ---
    Stage stage() const { return m_stage; }
    void setStage(Stage s) { m_stage = s; }

    // --- Outline ---
    QJsonObject outline() const { return m_outline; }
    void setOutline(const QJsonObject &o) { m_outline = o; }
    bool hasOutline() const { return !m_outline.isEmpty(); }

    // --- Article ---
    QString articleContent() const { return m_articleContent; }
    void setArticleContent(const QString &a) { m_articleContent = a; }
    bool hasArticle() const { return !m_articleContent.isEmpty(); }

    // --- Serialization ---
    QJsonObject toJson() const;
    static WritingState fromJson(const QJsonObject &json);

    /**
     * 从 QVariantHash params 中提取 WritingState。
     * 读取 params["writing_state_json"]（JSON 字符串）。
     * 若不存在则返回默认 GenerateOutline 状态。
     */
    static WritingState fromParams(const QVariantHash &params);

    /**
     * 将 WritingState 序列化为 JSON 字符串，用于存入 params。
     */
    QString toJsonString() const;

    /**
     * 构造用于注入 LLM 系统上下文的简短状态描述，供 WritingMasterAgent 使用。
     * 返回 [WRITING_PROJECT] ... [/WRITING_PROJECT] 格式字符串。
     */
    QString buildContextMessage() const;

private:
    Stage       m_stage          = Stage::GenerateOutline;
    QJsonObject m_outline;
    QString     m_articleContent;
};

} // namespace uos_ai

#endif // WRITINGSTATE_H
