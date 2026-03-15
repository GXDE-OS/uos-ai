#include "articlemodel.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStringList>

namespace uos_ai {

// ---------------------------------------------------------------------------
// fromMarkdown
// ---------------------------------------------------------------------------
ArticleModel ArticleModel::fromMarkdown(const QString &markdown)
{
    ArticleModel model;

    // 正则匹配 Markdown 标题行，可选尾部 [ID: xxx] 注记
    static QRegularExpression headingRe(
        R"(^(#{1,6})\s+(.*?)(?:\s+\[ID:\s*([^\]]+)\])?\s*$)",
        QRegularExpression::MultilineOption);

    QRegularExpressionMatchIterator it = headingRe.globalMatch(markdown);
    QList<QPair<int, Section>> rawSections; // <startPos, Section>

    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        Section sec;
        sec.level   = m.captured(1).length();
        sec.title   = m.captured(2).trimmed();
        sec.id      = m.captured(3).trimmed(); // 可能为空
        rawSections.append({m.capturedStart(), sec});
    }

    if (rawSections.isEmpty())
        return model;

    // 为没有 ID 的节分配位置 ID（s1, s2, s2_1 ...）
    // 使用计数栈按层级跟踪编号
    QList<int> counters = {0, 0, 0, 0, 0, 0, 0}; // counters[level], level in [1..6]
    int lastLevel = 0;

    for (auto &pair : rawSections) {
        Section &sec = pair.second;
        int lv = sec.level;

        if (sec.id.isEmpty()) {
            // 重置所有深于当前层的计数器
            for (int i = lv + 1; i <= 6; ++i)
                counters[i] = 0;
            counters[lv]++;

            // 构建 ID：s1 / s2_1 / s2_1_1 ...
            QString id = "s";
            for (int i = 1; i <= lv; ++i) {
                if (i > 1) id += "_";
                id += QString::number(counters[i]);
            }
            sec.id = id;
        }
        lastLevel = lv;
    }
    Q_UNUSED(lastLevel)

    // 提取各节的正文（从本节标题行之后到下一节标题行之前）
    for (int i = 0; i < rawSections.size(); ++i) {
        Section sec  = rawSections[i].second;
        int startPos = rawSections[i].first;

        // 找本节标题行的结束位置
        int headEnd = markdown.indexOf('\n', startPos);
        if (headEnd == -1) headEnd = markdown.length();
        else headEnd++; // 包含换行符

        // 下一节的开始位置（或文档末尾）
        int contentEnd = (i + 1 < rawSections.size())
                             ? rawSections[i + 1].first
                             : markdown.length();

        sec.content = markdown.mid(headEnd, contentEnd - headEnd).trimmed();
        model.m_sections.append(sec);
    }

    return model;
}

// ---------------------------------------------------------------------------
// fromOutline
// ---------------------------------------------------------------------------
ArticleModel ArticleModel::fromOutline(const QJsonObject &outline)
{
    ArticleModel model;

    // 根节点 title 作为标题（level=1），其 content 数组是子节点
    Section rootSec;
    rootSec.id      = "s0_title";
    rootSec.level   = 1;
    rootSec.title   = outline.value("title").toString();
    rootSec.content = QString();

    // 根 title 是文章总标题，不纳入章节列表（由 ReportWriterAgent 生成 # 标题）
    // 仅收集 content 数组下的子节点
    QJsonArray children = outline.value("content").toArray();
    int counter = 0;
    for (const QJsonValue &child : children) {
        collectOutlineSections(child.toObject(), "", 1, counter, model.m_sections);
    }

    return model;
}

void ArticleModel::collectOutlineSections(const QJsonObject &node,
                                          const QString &parentId,
                                          int level,
                                          int &counter,
                                          QList<Section> &sections)
{
    counter++;
    Section sec;
    sec.level   = level;
    sec.title   = node.value("title").toString();
    sec.content = QString();

    // 构建 ID
    sec.id = parentId.isEmpty()
                 ? QString("s%1").arg(counter)
                 : QString("%1_%2").arg(parentId).arg(counter);

    sections.append(sec);

    QJsonArray children = node.value("content").toArray();
    int childCounter = 0;
    for (const QJsonValue &child : children) {
        collectOutlineSections(child.toObject(), sec.id, level + 1, childCounter, sections);
    }
}

// ---------------------------------------------------------------------------
// sectionById / updateSection / deleteSection
// ---------------------------------------------------------------------------
const ArticleModel::Section *ArticleModel::sectionById(const QString &id) const
{
    for (const Section &sec : m_sections) {
        if (sec.id == id)
            return &sec;
    }
    return nullptr;
}

bool ArticleModel::updateSection(const QString &id, const QString &newContent)
{
    for (Section &sec : m_sections) {
        if (sec.id == id) {
            sec.content = newContent;
            return true;
        }
    }
    return false;
}

bool ArticleModel::updateSectionTitle(const QString &id, const QString &newTitle)
{
    for (Section &sec : m_sections) {
        if (sec.id == id) {
            sec.title = newTitle;
            return true;
        }
    }
    return false;
}

bool ArticleModel::deleteSection(const QString &id)
{
    for (int i = 0; i < m_sections.size(); ++i) {
        if (m_sections[i].id == id) {
            m_sections.removeAt(i);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
QString ArticleModel::toMarkdown() const
{
    QString result;
    for (const Section &sec : m_sections) {
        QString heading(sec.level, '#');
        result += QString("%1 %2\n\n%3\n\n").arg(heading, sec.title, sec.content);
    }
    return result.trimmed();
}

QString ArticleModel::toAnnotatedMarkdown() const
{
    QString result;
    for (const Section &sec : m_sections) {
        QString heading(sec.level, '#');
        result += QString("%1 %2 [ID: %3]\n\n%4\n\n").arg(heading, sec.title, sec.id, sec.content);
    }
    return result.trimmed();
}

// ---------------------------------------------------------------------------
// JSON serialization
// ---------------------------------------------------------------------------
QJsonObject ArticleModel::toJson() const
{
    QJsonArray arr;
    for (const Section &sec : m_sections) {
        QJsonObject obj;
        obj["id"]      = sec.id;
        obj["level"]   = sec.level;
        obj["title"]   = sec.title;
        obj["content"] = sec.content;
        arr.append(obj);
    }
    QJsonObject result;
    result["sections"] = arr;
    return result;
}

ArticleModel ArticleModel::fromJson(const QJsonObject &json)
{
    ArticleModel model;
    QJsonArray arr = json.value("sections").toArray();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        Section sec;
        sec.id      = obj.value("id").toString();
        sec.level   = obj.value("level").toInt(1);
        sec.title   = obj.value("title").toString();
        sec.content = obj.value("content").toString();
        model.m_sections.append(sec);
    }
    return model;
}

} // namespace uos_ai
