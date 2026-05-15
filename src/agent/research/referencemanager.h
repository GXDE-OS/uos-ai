#ifndef REFERENCEMANAGER_H
#define REFERENCEMANAGER_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QHash>

namespace uos_ai {

/**
 * ReferenceManager: 统一管理研究引用的分配、去重、格式化和按 ID 检索
 *
 * 存储每条引用的完整信息（含原始内容和分析后内容），
 * 支持 Writer 通过 retrieve 工具按 ID 精确检索。
 */
class ReferenceManager
{
public:
    ReferenceManager() = default;

    void reset(int offset = 1);

    int offset() const { return m_refOffset; }
    QJsonArray references() const { return m_references; }

    /**
     * 查找已有引用的 ID（按 source URL 去重）。
     * 若不存在返回空字符串。
     */
    QString findBySource(const QString &source) const;

    /**
     * 为一条搜索结果分配引用 ID。
     * 已存在则返回旧 ID，否则创建新引用并追加。
     * item 需包含 STR_KEY_URL / STR_KEY_TITLE 等字段。
     * 同时存储 content 和 cleaned_content 供 retrieve 工具使用。
     */
    QString addReference(const QJsonObject &item);

    /**
     * 批量分配引用并生成 "[ref_N]\nContent: ...\n" 格式文本。
     * @param results    搜索结果数组
     * @param contentKey 正文字段的 JSON 键名
     */
    QString formatResults(const QJsonArray &results, const QString &contentKey = {});

    /**
     * 检索所有引用的详细内容。
     * 返回格式化的材料文本，供 Writer 直接注入 prompt 使用。
     * 优先返回 cleaned_content，fallback 到 content，再 fallback 到 snippet。
     */
    QString retrieveAll() const;

    /**
     * 按引用 ID 列表检索详细内容。
     * 返回格式化的材料文本，供 Writer 的 retrieve 工具使用。
     * 优先返回 cleaned_content，fallback 到 content，再 fallback 到 snippet。
     */
    QString retrieveByIds(const QStringList &refIds) const;

    /**
     * 构建 <material> 内部的引用摘要，使用 <ref_N> 标签包裹（内部 ID）。
     * 供 Writer 的 user prompt 使用，让 LLM 知道有哪些材料可以 retrieve。
     * LLM 写作时引用格式为 <cite id="ref_N">（无闭合标签）。
     */
    QString buildMaterialSection() const;

    /**
     * 仅包含指定 ref_id 列表中的引用摘要。
     * refIds 为空时返回空字符串（不展示任何素材）。
     */
    QString buildMaterialSection(const QStringList &refIds) const;

    /**
     * 对指定引用按需抓取网页全文。
     * 仅抓取 content 字段为空且 URL 为 http(s) 的引用。
     * 若 refIds 为空则抓取所有引用。
     */
    void scrapeSelectedRefs(const QStringList &refIds = {});

    /**
     * 按 ref_id 查找单条引用。未找到返回空 QJsonObject。
     */
    QJsonObject findById(const QString &refId) const;

private:
    QString resolveContent(const QJsonObject &ref) const;
    QString formatReference(const QJsonObject &ref) const;
    static QString formatMaterialEntry(const QString &refId, const QJsonObject &ref);

    QJsonArray m_references;
    QMap<QString, QString> m_sourceIndex;  // source URL -> ref_id (O(1) dedup)
    QHash<QString, int> m_idIndex;         // ref_id -> index in m_references (O(1) lookup)
    int m_refOffset = 1;
};

} // namespace uos_ai

#endif // REFERENCEMANAGER_H
