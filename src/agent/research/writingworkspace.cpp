#include "writingworkspace.h"
#include "articlemodel.h"
#include "tools/iconstore.h"
#include "global_key_define.h"
#include "research_key_define.h"

#include <QJsonDocument>
#include <QJsonArray>

namespace uos_ai {

WritingWorkspace::WritingWorkspace()
{
}

void WritingWorkspace::setReferences(const QJsonArray &refs)
{
    m_references = refs;
    m_metaDirty = true;
}

void WritingWorkspace::appendReferences(const QJsonArray &newRefs)
{
    for (const auto &ref : newRefs)
        m_references.append(ref);
    m_metaDirty = true;
}

// ---------------------------------------------------------------------------
// Multi-article management
// ---------------------------------------------------------------------------

void WritingWorkspace::setActiveArticleId(const QString &articleId)
{
    m_activeArticleId = articleId;
    m_metaDirty = true;
}

Article *WritingWorkspace::activeArticle()
{
    if (m_activeArticleId.isEmpty())
        return nullptr;
    auto it = m_articles.find(m_activeArticleId);
    return it != m_articles.end() ? &it.value() : nullptr;
}

const Article *WritingWorkspace::activeArticle() const
{
    if (m_activeArticleId.isEmpty())
        return nullptr;
    auto it = m_articles.constFind(m_activeArticleId);
    return it != m_articles.constEnd() ? &it.value() : nullptr;
}

Article *WritingWorkspace::article(const QString &articleId)
{
    auto it = m_articles.find(articleId);
    return it != m_articles.end() ? &it.value() : nullptr;
}

const Article *WritingWorkspace::article(const QString &articleId) const
{
    auto it = m_articles.constFind(articleId);
    return it != m_articles.constEnd() ? &it.value() : nullptr;
}

QStringList WritingWorkspace::articleIds() const
{
    return m_articles.keys();
}

QString WritingWorkspace::addArticle(const QString &title)
{
    Article art = Article::create(title);
    QString artId = art.id();
    m_articles.insert(artId, art);
    m_activeArticleId = artId;
    m_metaDirty = true;
    return artId;
}

bool WritingWorkspace::removeArticle(const QString &articleId)
{
    if (!m_articles.contains(articleId))
        return false;

    m_articles.remove(articleId);
    if (m_activeArticleId == articleId)
        m_activeArticleId.clear();
    m_metaDirty = true;
    return true;
}

bool WritingWorkspace::hasArticleContent() const
{
    const Article *art = activeArticle();
    return art && art->hasContent();
}

void WritingWorkspace::insertArticle(const Article &art)
{
    m_articles.insert(art.id(), art);
}

// ---------------------------------------------------------------------------
// Serialization (transient fields are NOT included)
// ---------------------------------------------------------------------------

QJsonObject WritingWorkspace::toMetaJson() const
{
    QJsonObject obj;
    obj[STR_KEY_CONVERSATION_ID]         = m_conversationId;
    obj[STR_KEY_REFERENCES]              = m_references;
    obj[STR_RESEARCH_ACTIVE_ARTICLE_ID]  = m_activeArticleId;

    QJsonArray idsArr;
    for (auto it = m_articles.constBegin(); it != m_articles.constEnd(); ++it)
        idsArr.append(it.key());
    obj[STR_RESEARCH_ARTICLE_IDS] = idsArr;

    return obj;
}

QJsonObject WritingWorkspace::toJson() const
{
    QJsonObject obj;
    obj[STR_KEY_CONVERSATION_ID]        = m_conversationId;
    obj[STR_KEY_REFERENCES]             = m_references;
    obj[STR_RESEARCH_ACTIVE_ARTICLE_ID] = m_activeArticleId;

    QJsonObject articlesObj;
    for (auto it = m_articles.constBegin(); it != m_articles.constEnd(); ++it)
        articlesObj[it.key()] = it.value().toJson();
    obj[STR_RESEARCH_ARTICLES] = articlesObj;

    return obj;
}

WritingWorkspace WritingWorkspace::fromJson(const QJsonObject &json)
{
    WritingWorkspace ws;
    ws.m_conversationId  = json.value(STR_KEY_CONVERSATION_ID).toString();
    ws.m_references      = json.value(STR_KEY_REFERENCES).toArray();
    ws.m_activeArticleId = json.value(STR_RESEARCH_ACTIVE_ARTICLE_ID).toString();

    QJsonObject articlesObj = json.value(STR_RESEARCH_ARTICLES).toObject();
    for (auto it = articlesObj.constBegin(); it != articlesObj.constEnd(); ++it)
        ws.m_articles.insert(it.key(), Article::fromJson(it.value().toObject()));

    return ws;
}

// ---------------------------------------------------------------------------
// Context building
// ---------------------------------------------------------------------------

QString WritingWorkspace::buildContextMessage() const
{
    const Article *art = activeArticle();

    if (!art || !art->hasContent())
        return QString();

    QString structure;
    ArticleModel model = ArticleModel::fromMarkdown(art->currentContent());
    if (!model.isEmpty()) {
        for (const auto &sec : model.sections())
            structure += QString("%1 %2\n").arg(QString(sec.level, '#'), sec.title);
    }
    return QString("[WRITING_PROJECT]\nstage: adjusting\narticle_structure:\n%1[/WRITING_PROJECT]")
            .arg(structure);
}

QJsonArray WritingWorkspace::articleOrderedReferences(const QString &articleId) const
{
    const Article *art = article(articleId);
    if (!art)
        art = activeArticle();
    if (!art || art->refIds().isEmpty())
        return QJsonArray();

    QHash<QString, QJsonObject> refMap;
    for (const auto &val : m_references) {
        QJsonObject ref = val.toObject();
        refMap[ref.value(STR_KEY_ID).toString()] = ref;
    }

    QJsonArray ordered;
    const QStringList &refIds = art->refIds();
    for (int i = 0; i < refIds.size(); ++i) {
        const QJsonObject &ref = refMap.value(refIds[i]);
        if (ref.isEmpty())
            continue;
        QJsonObject item;
        const QString iconKey  = ref.value(STR_KEY_ICON).toString();
        const QString iconPath = IconStore::instance()->iconPath(iconKey);
        item[STR_KEY_INDEX]   = i + 1;
        item[STR_KEY_TITLE]   = ref.value(STR_KEY_TITLE);
        item[STR_KEY_URL]     = ref.value(STR_KEY_URL);
        item[STR_KEY_WEBSITE] = ref.value(STR_KEY_WEBSITE);
        item[STR_KEY_ICON]    = iconPath;
        item[STR_KEY_SNIPPET] = ref.value(STR_KEY_SNIPPET);
        ordered.append(item);
    }
    return ordered;
}

} // namespace uos_ai
