#ifndef ARTICLEMODEL_H
#define ARTICLEMODEL_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

namespace uos_ai {

/**
 * ArticleModel: 带稳定 Section ID 的结构化文章模型
 *
 * 解决 ArticleModificationAgent 基于正则 + 模糊标题匹配的脆弱改法：
 * 每个章节拥有稳定的 ID（与大纲共享，如 "s1", "s2_1"），
 * 修改时直接通过 ID 定位，无需重新解析整篇 Markdown。
 *
 * Section ID 命名规则：
 *   根级节点 → "s1", "s2", "s3"...
 *   子节点   → "s2_1", "s2_2", "s2_2_1"...
 */
class ArticleModel
{
public:
    struct Section {
        QString id;       // 稳定 ID，如 "s1", "s2_1"
        int     level;    // Markdown 标题级别 (1-6)
        QString title;    // 标题文本（不含 # 前缀）
        QString content;  // 本节正文（不含标题行）
    };

    ArticleModel() = default;

    /**
     * 从 Markdown 字符串构建 ArticleModel。
     * 若 Markdown 中已含 [ID: xxx] 注记则复用该 ID，
     * 否则按位置自动分配 ID（s1, s2, s2_1 ...）。
     */
    static ArticleModel fromMarkdown(const QString &markdown);

    /**
     * 从大纲 JSON 构建 ArticleModel（章节内容为空）。
     * 大纲结构与 OutlineAgent 输出格式一致：
     * { "title": "...", "content": [ { "title": "...", "content": [...] } ] }
     */
    static ArticleModel fromOutline(const QJsonObject &outline);

    bool isEmpty() const { return m_sections.isEmpty(); }
    const QList<Section> &sections() const { return m_sections; }

    // 按 ID 查找
    const Section *sectionById(const QString &id) const;

    // 按 ID 更新正文
    bool updateSection(const QString &id, const QString &newContent);

    // 按 ID 更新标题文本（不含 # 前缀）
    bool updateSectionTitle(const QString &id, const QString &newTitle);

    // 按 ID 删除节（保留其子节，子节内容并入前一个同级节末尾）
    bool deleteSection(const QString &id);

    // 渲染为标准 Markdown（无 ID 注记）
    QString toMarkdown() const;

    // 渲染为带 [ID: xxx] 注记的 Markdown，供 LLM 使用，便于精确定位章节
    // 示例输出：## 核心分析 [ID: s2]\n正文...
    QString toAnnotatedMarkdown() const;

    // JSON 序列化，存入 WritingState
    QJsonObject toJson() const;
    static ArticleModel fromJson(const QJsonObject &json);

private:
    /**
     * 递归遍历大纲 JSON，收集 Section 列表。
     * @param node      当前大纲节点
     * @param parentId  父节点 ID 前缀（根节点为空）
     * @param level     当前标题层级
     * @param counter   同级计数器（引用，自增）
     * @param sections  输出列表
     */
    static void collectOutlineSections(const QJsonObject &node,
                                       const QString &parentId,
                                       int level,
                                       int &counter,
                                       QList<Section> &sections);

    QList<Section> m_sections;
};

} // namespace uos_ai

#endif // ARTICLEMODEL_H
