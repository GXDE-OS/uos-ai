#ifndef WORKSPACESTORE_H
#define WORKSPACESTORE_H

#include "writingworkspace.h"

#include <QString>
#include <QHash>
#include <QList>
#include <QMutex>

namespace uos_ai {

/**
 * WorkspaceStore: WritingWorkspace 持久化存储（JSON 文件）
 *
 * 存储路径：~/.cache/deepin/uos-ai-assistant/workspace/ai-writing/
 * 每个 Workspace 一个子目录，目录名为 conversationId，
 * 目录内固定文件名为 workspace.json。
 * 原子写入：先写临时文件再 rename。
 */
class WorkspaceStore
{
public:
    static WorkspaceStore *instance();

    bool save(const WritingWorkspace &ws);
    /** 仅保存指定文章文件，并更新 workspace.json 元数据（不重写其他文章）。*/
    bool saveArticle(const QString &convId, const Article &article);
    WritingWorkspace load(const QString &convId);
    bool remove(const QString &convId);

    WritingWorkspace findByConversationId(const QString &convId);

    /** 加载全部 workspace */
    QList<WritingWorkspace> listAll();

    /**
     * 将指定文章内容 + 引用列表构建为 Markdown，弹出文件保存对话框写入本地。
     * @param format 保存格式："md"（Markdown）/ "docx"（Word）/ "pdf"（PDF），空字符串默认为 md
     * @return true 表示用户确认并成功写入，false 表示取消或写入失败
     */
    bool exportArticleToFile(const QString &convId, const QString &articleId, const QString &format);

private:
    WorkspaceStore();

    QString storagePath() const;
    QString filePath(const QString &convId) const;
    QString articlesDir(const QString &convId) const;
    QString articleFilePath(const QString &convId, const QString &articleId) const;

    bool saveMetaLocked(const WritingWorkspace &ws);
    bool saveArticleLocked(const QString &convId, const Article &article);
    WritingWorkspace loadLocked(const QString &convId);

    mutable QMutex m_mutex;
    QHash<QString, WritingWorkspace> m_cache;   // convId -> workspace
};

} // namespace uos_ai

#endif // WORKSPACESTORE_H
