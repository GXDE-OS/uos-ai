#include "article.h"
#include "research_key_define.h"
#include "global_key_define.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

namespace uos_ai {

// ---------------------------------------------------------------------------
// ArticleVersion
// ---------------------------------------------------------------------------

QJsonObject ArticleVersion::toJson() const
{
    QJsonObject obj;
    obj[STR_RESEARCH_VERSION]            = version;
    obj[STR_KEY_CONTENT]                 = content;
    obj[STR_RESEARCH_CHANGE_DESC]        = changeDescription;
    obj[STR_KEY_CREATED_AT]              = createdAt.toString(Qt::ISODate);
    return obj;
}

ArticleVersion ArticleVersion::fromJson(const QJsonObject &json)
{
    ArticleVersion v;
    v.version           = json.value(STR_RESEARCH_VERSION).toInt();
    v.content           = json.value(STR_KEY_CONTENT).toString();
    v.changeDescription = json.value(STR_RESEARCH_CHANGE_DESC).toString();
    v.createdAt         = QDateTime::fromString(json.value(STR_KEY_CREATED_AT).toString(), Qt::ISODate);
    return v;
}

// ---------------------------------------------------------------------------
// Article
// ---------------------------------------------------------------------------

Article::Article()
    : m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(m_createdAt)
{
}

Article Article::create(const QString &title)
{
    Article art;
    art.m_id    = QUuid::createUuid().toString(QUuid::WithoutBraces);
    art.m_title = title;
    art.m_dirty = true;
    return art;
}

void Article::setTitle(const QString &title)
{
    m_title = title;
    touch();
}

void Article::setOutline(const QJsonObject &outline)
{
    m_outline = outline;
    touch();
}

void Article::setRefIds(const QStringList &ids)
{
    m_refIds = ids;
    touch();
}

void Article::touch()
{
    m_updatedAt = QDateTime::currentDateTime();
    m_dirty = true;
}

QString Article::currentContent() const
{
    const ArticleVersion *v = version(m_currentVersion);
    return v ? v->content : QString();
}

bool Article::hasContent() const
{
    return !currentContent().isEmpty();
}

const ArticleVersion *Article::version(int ver) const
{
    for (const auto &v : m_versions) {
        if (v.version == ver)
            return &v;
    }
    return nullptr;
}

void Article::commitVersion(const QString &content, const QString &changeDesc)
{
    ArticleVersion v;
    v.version           = m_currentVersion + 1;
    v.content           = content;
    v.changeDescription = changeDesc;
    v.createdAt         = QDateTime::currentDateTime();

    m_versions.append(v);
    m_currentVersion = v.version;

    // Evict oldest versions if over limit
    while (m_versions.size() > MAX_VERSIONS)
        m_versions.removeFirst();

    touch();
}

void Article::updateContent(const QString &content)
{
    if (m_versions.isEmpty()) {
        ArticleVersion v;
        v.version           = 1;
        v.content           = content;
        v.changeDescription = QStringLiteral("initial");
        v.createdAt         = QDateTime::currentDateTime();
        m_versions.append(v);
        m_currentVersion    = 1;
    } else {
        for (auto &v : m_versions) {
            if (v.version == m_currentVersion) {
                v.content = content;
                break;
            }
        }
    }
    touch();
}

bool Article::rollbackTo(int ver)
{
    const ArticleVersion *target = version(ver);
    if (!target)
        return false;

    QString desc = QString("rollback to version %1").arg(ver);
    commitVersion(target->content, desc);
    return true;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

QJsonObject Article::toJson() const
{
    QJsonObject obj;
    obj[STR_KEY_ID]                       = m_id;
    obj[STR_KEY_TITLE]                    = m_title;
    obj[STR_KEY_OUTLINE]                  = m_outline;
    obj[STR_KEY_CREATED_AT]               = m_createdAt.toString(Qt::ISODate);
    obj[STR_KEY_UPDATED_AT]               = m_updatedAt.toString(Qt::ISODate);
    obj[STR_RESEARCH_CURRENT_VERSION]     = m_currentVersion;
    obj[STR_RESEARCH_REF_IDS]             = QJsonArray::fromStringList(m_refIds);

    QJsonArray versionsArr;
    for (const auto &v : m_versions)
        versionsArr.append(v.toJson());
    obj[STR_RESEARCH_VERSIONS] = versionsArr;

    return obj;
}

Article Article::fromJson(const QJsonObject &json)
{
    Article art;
    art.m_id             = json.value(STR_KEY_ID).toString();
    art.m_title          = json.value(STR_KEY_TITLE).toString();
    art.m_outline        = json.value(STR_KEY_OUTLINE).toObject();
    art.m_createdAt      = QDateTime::fromString(json.value(STR_KEY_CREATED_AT).toString(), Qt::ISODate);
    art.m_updatedAt      = QDateTime::fromString(json.value(STR_KEY_UPDATED_AT).toString(), Qt::ISODate);
    art.m_currentVersion = json.value(STR_RESEARCH_CURRENT_VERSION).toInt();
    for (const auto &val : json.value(STR_RESEARCH_REF_IDS).toArray())
        art.m_refIds.append(val.toString());

    QJsonArray versionsArr = json.value(STR_RESEARCH_VERSIONS).toArray();
    for (const auto &val : versionsArr)
        art.m_versions.append(ArticleVersion::fromJson(val.toObject()));

    return art;
}

} // namespace uos_ai
