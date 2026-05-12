#ifndef ARTICLE_H
#define ARTICLE_H

#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>

namespace uos_ai {

/**
 * ArticleVersion: 文章的一个全量快照版本
 */
struct ArticleVersion {
    int version = 0;
    QString content;
    QString changeDescription;
    QDateTime createdAt;

    QJsonObject toJson() const;
    static ArticleVersion fromJson(const QJsonObject &json);
};

/**
 * Article: 纯内容容器 + 版本管理
 *
 * 不绑定大纲或写作模式，可由任意写作流程产生。
 * 通过 commitVersion() 统一入口创建新版本，
 * 适用于 pipeline 生成和用户手动编辑两种场景。
 */
class Article
{
public:
    Article();

    static Article create(const QString &title);

    // --- 标识 & 元信息 ---
    QString id() const { return m_id; }
    QString title() const { return m_title; }
    void setTitle(const QString &title);
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime updatedAt() const { return m_updatedAt; }

    // --- 当前内容（快捷访问）---
    QString currentContent() const;
    bool hasContent() const;

    // --- 版本管理 ---
    int currentVersionNumber() const { return m_currentVersion; }
    int versionCount() const { return m_versions.size(); }
    QList<ArticleVersion> versions() const { return m_versions; }
    const ArticleVersion *version(int ver) const;

    /**
     * 提交新版本。自动递增版本号，超过 MAX_VERSIONS 时淘汰最早版本。
     */
    void commitVersion(const QString &content, const QString &changeDesc);

    /**
     * 原地更新当前版本内容，不创建新版本。
     * 若尚无任何版本，则初始化为版本 1。
     */
    void updateContent(const QString &content);

    /**
     * 回退到指定版本。以目标版本内容创建新快照，历史不删除。
     * @return false 表示目标版本不存在
     */
    bool rollbackTo(int ver);

    bool isValid() const { return !m_id.isEmpty(); }

    // --- 脏标记（用于增量持久化）---
    bool isDirty() const { return m_dirty; }
    void clearDirty() const { m_dirty = false; }

    // --- 大纲（属于本文章的写作规划，随文章持久化）---
    QJsonObject outline() const { return m_outline; }
    void setOutline(const QJsonObject &outline);
    bool hasOutline() const { return !m_outline.isEmpty(); }

    // --- 引用列表（按文中出现顺序排列的 ref_id）---
    QStringList refIds() const { return m_refIds; }
    void setRefIds(const QStringList &ids);

    // --- 序列化 ---
    QJsonObject toJson() const;
    static Article fromJson(const QJsonObject &json);

private:
    void touch();

    QString m_id;
    QString m_title;
    QJsonObject m_outline;
    QStringList m_refIds;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    int m_currentVersion = 0;
    QList<ArticleVersion> m_versions;

    mutable bool m_dirty = false;

    static constexpr int MAX_VERSIONS = 30;
};

} // namespace uos_ai

#endif // ARTICLE_H
