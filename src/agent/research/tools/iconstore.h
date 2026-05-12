#ifndef ICONSTORE_H
#define ICONSTORE_H

#include <QString>
#include <QByteArray>
#include <QImage>
#include <QHash>
#include <QMutex>

namespace uos_ai {

/**
 * IconStore: 引用图标的本地文件存储
 *
 * 将图标保存到 ~/.cache/deepin/uos-ai-assistant/workspace/ai-writing/web-icons/，
 * 以域名或文件扩展名为键去重，避免重复下载/生成。
 *
 * 引用的 icon 字段从 Base64 改为存储图标键名（如 "example.com"），
 * 前端通过 iconPath() 解析为绝对路径渲染。
 */
class IconStore
{
public:
    static IconStore *instance();

    /**
     * 存储目录的绝对路径
     */
    QString storagePath() const;

    /**
     * 从原始二进制数据保存图标。
     * 如果同 key 已存在则跳过写入。
     * @param key  图标键名（如域名 "example.com" 或 "ext_pdf"）
     * @param data PNG 图片原始数据
     * @return 图标键名（即传入的 key），失败返回空
     */
    QString saveFromData(const QString &key, const QByteArray &data);

    /**
     * 从 QImage 保存图标。
     * @param key   图标键名
     * @param image 图标图像
     * @return 图标键名，失败返回空
     */
    QString saveFromImage(const QString &key, const QImage &image);

    /**
     * 获取图标文件的绝对路径。文件不存在时返回空。
     */
    QString iconPath(const QString &key) const;

    /**
     * 检查指定 key 的图标是否已存在。
     */
    bool exists(const QString &key) const;

    /**
     * 从 URL 提取域名作为图标键。
     * "https://www.example.com/page" -> "example.com"
     */
    static QString domainKey(const QString &url);

    /**
     * 从文件扩展名生成图标键。
     * "/path/to/file.pdf" -> "ext_pdf"
     */
    static QString extensionKey(const QString &filePath);

private:
    IconStore();
    bool ensureDir() const;
    QString filePath(const QString &key) const;

    mutable QMutex m_mutex;
    mutable QHash<QString, bool> m_existsCache;
};

} // namespace uos_ai

#endif // ICONSTORE_H
