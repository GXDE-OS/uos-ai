#ifndef WRITINGWORKSPACE_H
#define WRITINGWORKSPACE_H

#include "article.h"

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

namespace uos_ai {

/**
 * WritingWorkspace: 一次写作任务的完整数据容器
 *
 * 包含 workspace 级规划数据（引用）和多篇独立文章。
 * 通过 conversationId 与对话历史关联，但数据独立存储。
 *
 * 瞬态字段仅在管线执行期间存活，不参与序列化。
 */
class WritingWorkspace
{
public:
    WritingWorkspace();

    // --- 标识 ---
    QString conversationId() const { return m_conversationId; }
    void setConversationId(const QString &convId) { m_conversationId = convId; m_metaDirty = true; }

    // --- workspace 级引用 ---
    QJsonArray references() const { return m_references; }
    void setReferences(const QJsonArray &refs);
    void appendReferences(const QJsonArray &newRefs);

    // --- 多文章管理 ---
    QString activeArticleId() const { return m_activeArticleId; }
    void setActiveArticleId(const QString &articleId);

    Article *activeArticle();
    const Article *activeArticle() const;

    Article *article(const QString &articleId);
    const Article *article(const QString &articleId) const;

    QStringList articleIds() const;
    int articleCount() const { return m_articles.size(); }
    bool hasArticles() const { return !m_articles.isEmpty(); }

    /**
     * 创建新文章并加入 workspace，返回新文章 ID。
     * 自动设为当前活跃文章。
     */
    QString addArticle(const QString &title);

    /**
     * 移除指定文章。如果移除的是活跃文章，activeArticleId 清空。
     */
    bool removeArticle(const QString &articleId);

    // --- 便捷方法：当前活跃文章是否有内容 ---
    bool hasArticleContent() const;

    /**
     * 将指定文章插入 workspace（用于分离存储加载时注入）。
     */
    void insertArticle(const Article &art);

    // --- 序列化 ---
    QJsonObject toJson() const;
    /** 仅包含元数据，articles 以 article_ids 数组替代（用于分离存储）。*/
    QJsonObject toMetaJson() const;
    static WritingWorkspace fromJson(const QJsonObject &json);

    /**
     * 构造用于注入 LLM 系统上下文的状态描述。
     * 基于当前活跃文章构建。
     * 返回 [WRITING_PROJECT] ... [/WRITING_PROJECT] 格式字符串。
     */
    QString buildContextMessage() const;

    /**
     * 返回指定文章的有序引用列表，按文中脚注出现顺序排列。
     * 每项包含 num（脚注编号）、title、url、website、icon、snippet。
     * 若文章无引用返回空数组。
     */
    QJsonArray articleOrderedReferences(const QString &articleId) const;

    // --- 脏标记（用于增量持久化）---
    bool isMetaDirty() const { return m_metaDirty; }
    void clearMetaDirty() const { m_metaDirty = false; }

    bool isValid() const { return !m_conversationId.isEmpty(); }

private:
    QString m_conversationId;

    // Workspace-level planning data
    QJsonArray m_references;

    // Multi-article
    QMap<QString, Article> m_articles;
    QString m_activeArticleId;

    mutable bool m_metaDirty = false;

};

} // namespace uos_ai

#endif // WRITINGWORKSPACE_H
