#include "referencemanager.h"
#include "tools/webscraper.h"
#include "global_key_define.h"
#include "research_key_define.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logResearch)

namespace uos_ai {

void ReferenceManager::reset(int offset)
{
    m_references = QJsonArray();
    m_sourceIndex.clear();
    m_idIndex.clear();
    m_refOffset = offset;
}

QString ReferenceManager::findBySource(const QString &source) const
{
    return m_sourceIndex.value(source);
}

QString ReferenceManager::addReference(const QJsonObject &item)
{
    QString source = item.value(STR_KEY_URL).toString();

    QString existing = findBySource(source);
    if (!existing.isEmpty())
        return existing;

    QString refId = QString("ref_%1").arg(m_refOffset++);
    QJsonObject refObj;
    refObj[STR_KEY_ID]      = refId;
    refObj[STR_KEY_TITLE]   = item.value(STR_KEY_TITLE).toString();
    refObj[STR_KEY_URL]     = source;
    refObj[STR_KEY_WEBSITE] = item.value(STR_KEY_WEBSITE).toString();
    refObj[STR_KEY_ICON]    = item.value(STR_KEY_ICON).toString();
    refObj[STR_KEY_SNIPPET] = item.value(STR_KEY_SNIPPET).toString();

    // Store full content for retrieve tool
    if (item.contains(STR_KEY_CONTENT))
        refObj[STR_KEY_CONTENT] = item.value(STR_KEY_CONTENT).toString();
    if (item.contains(STR_RESEARCH_CLEANED_CONTENT))
        refObj[STR_RESEARCH_CLEANED_CONTENT] = item.value(STR_RESEARCH_CLEANED_CONTENT).toString();

    m_idIndex[refId] = m_references.size();
    m_references.append(refObj);
    m_sourceIndex[source] = refId;
    return refId;
}

QString ReferenceManager::formatResults(const QJsonArray &results, const QString &contentKey)
{
    const QString &key = contentKey.isEmpty() ? QString(STR_KEY_CONTENT) : contentKey;
    QString output;
    for (const auto &val : results) {
        QJsonObject item = val.toObject();
        QString content = item.value(key).toString();
        QString refId = addReference(item);
        output += QString("[%1]\nContent: %2\n").arg(refId, content);
    }
    return output;
}

QString ReferenceManager::resolveContent(const QJsonObject &ref) const
{
    QString content = ref.value(STR_RESEARCH_CLEANED_CONTENT).toString();
    if (content.isEmpty())
        content = ref.value(STR_KEY_CONTENT).toString();
    if (content.isEmpty())
        content = ref.value(STR_KEY_SNIPPET).toString();
    return content;
}

QString ReferenceManager::formatMaterialEntry(const QString &refId, const QJsonObject &ref)
{
    QString titleStr = ref.value(STR_KEY_TITLE).toString();
    QString snippet = ref.value(STR_KEY_SNIPPET).toString();
    if (snippet.isEmpty())
        snippet = titleStr;
    return QString("<%1>\nTitle: %2\nSummary: %3\n</%1>\n")
                .arg(refId, titleStr, snippet.left(300));
}

QString ReferenceManager::formatReference(const QJsonObject &ref) const
{
    QString id = ref.value(STR_KEY_ID).toString();
    return QString("<%1>\nTitle: %2\nSource: %3\nContent:\n%4\n</%1>\n\n")
        .arg(id, ref.value(STR_KEY_TITLE).toString(),
             ref.value(STR_KEY_URL).toString(), resolveContent(ref));
}

QString ReferenceManager::retrieveAll() const
{
    QString output;
    for (const auto &val : m_references)
        output += formatReference(val.toObject());

    if (output.isEmpty())
        return QStringLiteral("No reference materials available.");

    return output;
}

QString ReferenceManager::retrieveByIds(const QStringList &refIds) const
{
    QString output;
    for (const QString &refId : refIds) {
        int idx = m_idIndex.value(refId, -1);
        if (idx < 0 || idx >= m_references.size())
            continue;
        output += formatReference(m_references[idx].toObject());
    }

    if (output.isEmpty())
        return QStringLiteral("No references found for the requested IDs.");

    return output;
}

QString ReferenceManager::buildMaterialSection() const
{
    if (m_references.isEmpty())
        return QString();

    QString material;
    for (const auto &val : m_references) {
        QJsonObject ref = val.toObject();
        material += formatMaterialEntry(ref.value(STR_KEY_ID).toString(), ref);
    }
    return material;
}

QString ReferenceManager::buildMaterialSection(const QStringList &refIds) const
{
    if (refIds.isEmpty())
        return QString();

    QString material;
    for (const QString &refId : refIds) {
        int idx = m_idIndex.value(refId, -1);
        if (idx < 0 || idx >= m_references.size())
            continue;
        material += formatMaterialEntry(refId, m_references[idx].toObject());
    }
    return material;
}

void ReferenceManager::scrapeSelectedRefs(const QStringList &refIds)
{
    WebScraper scraper;

    // Use index for O(1) lookup when refIds is provided
    QList<int> indicesToScrape;
    if (refIds.isEmpty()) {
        for (int i = 0; i < m_references.size(); ++i)
            indicesToScrape.append(i);
    } else {
        for (const QString &id : refIds) {
            int idx = m_idIndex.value(id, -1);
            if (idx >= 0)
                indicesToScrape.append(idx);
        }
    }

    for (int i : indicesToScrape) {
        if (i < 0 || i >= m_references.size())
            continue;
        QJsonObject ref = m_references[i].toObject();

        // Skip if already has content
        if (!ref.value(STR_KEY_CONTENT).toString().isEmpty())
            continue;

        // Skip non-web URLs (local files, etc.)
        QString url = ref.value(STR_KEY_URL).toString();
        if (!url.startsWith(QLatin1String("http")))
            continue;

        QString scraped = scraper.scrape(url);
        if (!scraped.isEmpty()) {
            ref[STR_KEY_CONTENT] = scraped;
            m_references[i] = ref;
            qCDebug(logResearch) << "Scraped content for" << ref.value(STR_KEY_ID).toString() << url.left(80);
        } else {
            qCWarning(logResearch) << "ReferenceManager: scrape failed for" << ref.value(STR_KEY_ID).toString() << url.left(80);
        }
    }
}

QJsonObject ReferenceManager::findById(const QString &refId) const
{
    int idx = m_idIndex.value(refId, -1);
    if (idx < 0 || idx >= m_references.size())
        return QJsonObject();
    return m_references[idx].toObject();
}

} // namespace uos_ai
