#include "workspacestore.h"
#include "research_key_define.h"
#include "tools/researchtools.h"
#include "global_key_define.h"

#include <DFileDialog>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QApplication>

DWIDGET_USE_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

WorkspaceStore *WorkspaceStore::instance()
{
    static WorkspaceStore store;
    return &store;
}

WorkspaceStore::WorkspaceStore()
{
}

QString WorkspaceStore::storagePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/workspace/ai-writing";
}

QString WorkspaceStore::filePath(const QString &convId) const
{
    return storagePath() + "/" + convId + "/workspace.json";
}

QString WorkspaceStore::articlesDir(const QString &convId) const
{
    return storagePath() + "/" + convId + "/articles";
}

QString WorkspaceStore::articleFilePath(const QString &convId, const QString &articleId) const
{
    return articlesDir(convId) + "/" + articleId + ".json";
}

// ---------------------------------------------------------------------------
// Private locked helpers (called with m_mutex held)
// ---------------------------------------------------------------------------

bool WorkspaceStore::saveMetaLocked(const WritingWorkspace &ws)
{
    const QString wsDir = storagePath() + "/" + ws.conversationId();
    if (!QDir(wsDir).mkpath(".")) {
        qCWarning(logResearch) << "WorkspaceStore: failed to create directory:" << wsDir;
        return false;
    }

    const QString path = filePath(ws.conversationId());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(logResearch) << "WorkspaceStore: failed to open for writing:" << path;
        return false;
    }

    QByteArray data = QJsonDocument(ws.toMetaJson()).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size())
        return false;

    return file.commit();
}

bool WorkspaceStore::saveArticleLocked(const QString &convId, const Article &article)
{
    if (!QDir(articlesDir(convId)).mkpath(".")) {
        qCWarning(logResearch) << "WorkspaceStore: failed to create articles dir for:" << convId;
        return false;
    }

    const QString path = articleFilePath(convId, article.id());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(logResearch) << "WorkspaceStore: failed to open article file for writing:" << path;
        return false;
    }

    QByteArray data = QJsonDocument(article.toJson()).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size())
        return false;

    return file.commit();
}

WritingWorkspace WorkspaceStore::loadLocked(const QString &convId)
{
    auto it = m_cache.constFind(convId);
    if (it != m_cache.constEnd())
        return it.value();

    QFile file(filePath(convId));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return WritingWorkspace();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull() || !doc.isObject()) {
        qCWarning(logResearch) << "WorkspaceStore: invalid JSON:" << filePath(convId)
                               << parseError.errorString();
        return WritingWorkspace();
    }

    QJsonObject obj = doc.object();
    WritingWorkspace ws = WritingWorkspace::fromJson(obj);
    if (!ws.isValid())
        return WritingWorkspace();

    if (!obj.contains(STR_RESEARCH_ARTICLES)) {
        // New format: articles stored in separate files, load each one
        const QJsonArray idsArr = obj.value(STR_RESEARCH_ARTICLE_IDS).toArray();
        for (const auto &val : idsArr) {
            const QString artId = val.toString();
            QFile artFile(articleFilePath(convId, artId));
            if (!artFile.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            QJsonDocument artDoc = QJsonDocument::fromJson(artFile.readAll());
            if (artDoc.isNull() || !artDoc.isObject())
                continue;
            Article art = Article::fromJson(artDoc.object());
            if (art.isValid())
                ws.insertArticle(art);
        }
    }

    m_cache[convId] = ws;
    return ws;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool WorkspaceStore::save(const WritingWorkspace &ws)
{
    QMutexLocker locker(&m_mutex);

    if (ws.isMetaDirty()) {
        if (!saveMetaLocked(ws))
            return false;
        ws.clearMetaDirty();
    }

    for (const QString &artId : ws.articleIds()) {
        const Article *art = ws.article(artId);
        if (art && art->isDirty()) {
            if (!saveArticleLocked(ws.conversationId(), *art))
                return false;
            art->clearDirty();
        }
    }

    m_cache[ws.conversationId()] = ws;
    return true;
}

bool WorkspaceStore::saveArticle(const QString &convId, const Article &article)
{
    QMutexLocker locker(&m_mutex);

    if (!saveArticleLocked(convId, article))
        return false;

    auto it = m_cache.find(convId);
    if (it != m_cache.end()) {
        if (Article *cached = it->article(article.id()))
            *cached = article;
    }

    return true;
}

WritingWorkspace WorkspaceStore::load(const QString &convId)
{
    QMutexLocker locker(&m_mutex);
    return loadLocked(convId);
}

bool WorkspaceStore::remove(const QString &convId)
{
    QMutexLocker locker(&m_mutex);

    m_cache.remove(convId);

    QDir wsDir(storagePath() + "/" + convId);
    return wsDir.removeRecursively();
}

WritingWorkspace WorkspaceStore::findByConversationId(const QString &convId)
{
    return load(convId);
}

QList<WritingWorkspace> WorkspaceStore::listAll()
{
    QMutexLocker locker(&m_mutex);

    QList<WritingWorkspace> result;

    QDir dir(storagePath());
    if (!dir.exists())
        return result;

    const QStringList convDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &convId : convDirs) {
        WritingWorkspace ws = loadLocked(convId);
        if (ws.isValid())
            result.append(ws);
    }

    return result;
}

bool WorkspaceStore::exportArticleToFile(const QString &convId, const QString &articleId, const QString &format)
{
    WritingWorkspace ws = findByConversationId(convId);
    if (!ws.isValid()) {
        qCWarning(logResearch) << "exportArticleToFile: workspace not found:" << convId;
        return false;
    }

    const Article *art = ws.article(articleId);
    if (!art)
        art = ws.activeArticle();
    if (!art) {
        qCWarning(logResearch) << "exportArticleToFile: article not found:" << articleId;
        return false;
    }

    // 构建内容：正文 + 参考资料
    // md：保留 [^n] 角标，用 [^n]: 定义式脚注
    // docx/pdf：去除正文中的 [^n] 角标，改用编号列表（转换工具不支持 Markdown 脚注语法）
    const bool isMd = (format == STR_RESEARCH_FORMAT_MD || format.isEmpty());
    QString markdown = art->currentContent();
    if (!isMd) {
        static const QRegularExpression kFootnoteRef(QStringLiteral(R"(\[\^\d+\])"));
        markdown.remove(kFootnoteRef);
    }

    markdown += "\n\n*" + QCoreApplication::translate("WorkspaceStore", "Note: Part of the document content may be generated by AI") + "*";

    QJsonArray refs = ws.articleOrderedReferences(art->id());
    if (!refs.isEmpty()) {
        markdown += "\n\n---\n\n## " + QCoreApplication::translate("WorkspaceStore", "References") + "\n\n";
        for (const QJsonValue &val : refs) {
            QJsonObject ref = val.toObject();
            int index     = ref.value(STR_KEY_INDEX).toInt();
            QString title = ref.value(STR_KEY_TITLE).toString();
            QString url   = ref.value(STR_KEY_URL).toString();

            if (isMd) {
                if (url.isEmpty())
                    markdown += QString("[^%1]: %2").arg(index).arg(title);
                else
                    markdown += QString("[^%1]: [%2](%3)").arg(index).arg(title).arg(url);
            } else {
                if (url.isEmpty())
                    markdown += QString("[%1] %2").arg(index).arg(title);
                else
                    markdown += QString("[%1] [%2](%3)").arg(index).arg(title).arg(url);
            }
            markdown += "\n";
        }
    }

    // 根据格式确定后缀和文件过滤器
    QString suffix;
    QString filter;
    if (format == STR_RESEARCH_FORMAT_DOCX) {
        suffix = STR_RESEARCH_FORMAT_DOCX;
        filter = QCoreApplication::translate("WorkspaceStore", "Word files (*.docx)");
    } else if (format == STR_RESEARCH_FORMAT_PDF) {
        suffix = STR_RESEARCH_FORMAT_PDF;
        filter = QCoreApplication::translate("WorkspaceStore", "PDF files (*.pdf)");
    } else {
        suffix = STR_RESEARCH_FORMAT_MD;
        filter = QCoreApplication::translate("WorkspaceStore","Markdown files (*.md)");
    }

    const QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                + "/" + art->title() + "." + suffix;
    QString savePath = DFileDialog::getSaveFileName(
                           qApp->activeWindow(),
                           QCoreApplication::translate("WorkspaceStore", "Save Article"),
                           defaultPath,
                           filter);
    if (savePath.isEmpty())
        return false;

    if (!savePath.endsWith("." + suffix, Qt::CaseInsensitive))
        savePath += "." + suffix;

    bool ok = false;
    if (suffix == STR_RESEARCH_FORMAT_DOCX) {
        ok = ResearchTools::md2Word(markdown, savePath);
    } else if (suffix == STR_RESEARCH_FORMAT_PDF) {
        ok = ResearchTools::md2Pdf(markdown, savePath);
    } else {
        QSaveFile file(savePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(markdown.toUtf8());
            ok = file.commit();
        } else {
            qCWarning(logResearch) << "exportArticleToFile: cannot open file:" << savePath;
        }
    }

    qCDebug(logResearch) << "exportArticleToFile: saved to" << savePath << "format:" << suffix << "ok:" << ok;
    return ok;
}

} // namespace uos_ai
