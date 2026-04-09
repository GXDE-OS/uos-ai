#ifndef SKILLS_MANAGER_H
#define SKILLS_MANAGER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QFileSystemWatcher>

class QDir;

namespace uos_ai {

/**
 * @brief 技能信息类
 *
 * 描述单个AI技能的元数据信息，支持延迟加载详细内容
 */
class SkillInfo
{
public:
    /**
     * @brief 默认构造函数，初始化空技能信息
     */
    SkillInfo()
        : enabled(false), m_detailsLoaded(false) {}

    /**
     * @brief 完整构造函数
     * @param name 技能名称
     * @param description 技能简短描述
     * @param path 技能文件路径
     * @param source 技能来源（如 uos-ai/claude/vscode）
     * @param enabled 是否启用该技能
     */
    SkillInfo(const QString &name, const QString &description,
              const QString &path, const QString &source, bool enabled = true)
        : name(name), description(description), path(path), source(source), enabled(enabled), m_detailsLoaded(false)
    {
    }

    /**
     * @brief 获取技能详细内容（延迟加载）
     * @return 技能的详细内容，首次访问时从文件加载
     */
    QString details() const
    {
        if (!m_detailsLoaded) {
            loadDetails();
        }
        return m_details;
    }

    /**
     * @brief 获取技能文件夹中的文件列表（延迟加载）
     * @return 文件的完整绝对路径列表，排除 LICENSE、README 系列文件和 SKILL.md
     */
    QStringList files() const
    {
        if (!m_filesLoaded) {
            loadFiles();
        }
        return m_files;
    }

    QString name; /* 技能名称 */
    QString description; /* 技能简短描述 */
    QString path; /* 技能文件路径 */
    QString source; /* 技能来源（uos-ai/claude/vscode/...） */
    bool enabled; /* 是否启用该技能 */

private:
    /**
     * @brief 从文件加载详细内容
     */
    void loadDetails() const;

    /**
     * @brief 加载文件列表
     */
    void loadFiles() const;

    /**
     * @brief 递归收集目录中的文件
     * @param dir 当前目录
     */
    void collectFiles(const QDir &dir) const;

    mutable QString m_details; /* 延迟加载的详细内容 */
    mutable bool m_detailsLoaded; /* 是否已加载详细内容 */
    mutable QStringList m_files; /* 延迟加载的文件列表 */
    mutable bool m_filesLoaded; /* 是否已加载文件列表 */
};

/**
 * @brief AI技能管理器
 *
 * 负责管理AI技能的加载、更新、查询，并提供技能列表的实时变更通知。
 * 支持从文件系统动态加载技能，并监控技能文件的变更自动刷新。
 */
class SkillsManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit SkillsManager(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~SkillsManager();

    /**
     * @brief 获取技能提示词摘要
     * @return 包含所有已启用技能列表的提示词字符串，可直接插入到系统提示词中
     *
     * 该方法将所有已启用的技能信息格式化为AI可理解的提示词格式
     */
    QString prompt() const;

    /**
     * @brief 读取指定技能的信息
     * @param skillName 技能名称
     * @return 技能信息对象，若技能不存在则返回空对象（SkillInfo().isValid() 为 false）
     */
    SkillInfo readSkill(const QString &skillName) const;

    /**
     * @brief 获取所有技能信息列表
     * @return 所有技能的列表，按来源和名称排序
     *
     * 排序规则：
     * 1. 一级排序：builtin > uos-ai > 其他来源
     * 2. 二级排序：在同一来源内，按技能名称排序
     */
    QList<SkillInfo> skills() const;

    /**
     * @brief 获取所有已启用的技能信息列表
     * @return 所有已启用技能的列表，按来源和名称排序
     *
     * 排序规则：
     * 1. 一级排序：builtin > uos-ai > 其他来源
     * 2. 二级排序：在同一来源内，按技能名称排序
     */
    QList<SkillInfo> enabledSkills() const;

    /**
     * @brief 设置技能的启用状态
     * @param skillName 技能名称
     * @param enabled 启用状态（true 启用，false 禁用）
     * @return 设置是否成功
     */
    bool setSkillEnabled(const QString &skillName, bool enabled);

    /**
     * @brief 重新加载技能列表
     *
     * 从技能目录重新扫描并加载所有技能文件
     */
    void reloadSkills();

    /**
     * @brief 添加一个新技能
     * @param path 技能文件夹路径、SKILL.md 文件路径或压缩包文件路径
     * @param errorMsg 可选输出参数，失败时填入用户可读的错误原因
     * @param outSkillName 可选输出参数，成功时填入 SKILL.md 中解析出的技能名称
     * @return 添加是否成功
     *
     * 支持传入技能文件夹或 SKILL.md 文件路径，将整个技能文件夹拷贝到 ~/.uos-ai/skills/ 目录下。
     * 如果同名技能已存在，则会覆盖。
     * 注意：此方法不会手动触发重载，依赖 QFileSystemWatcher 自动处理。
     */
    bool addSkill(const QString &path, QString *errorMsg = nullptr, QString *outSkillName = nullptr);

    /**
     * @brief 删除指定技能
     * @param skillName 技能名称
     * @return 删除是否成功
     *
     * 将技能文件夹移动到系统回收站 (~/.local/share/Trash/files)。
     * 仅处理来自 ~/.uos-ai/skills/ 的技能，其他来源的技能会拒绝删除。
     * 注意：此方法不会手动触发重载，依赖 QFileSystemWatcher 自动处理。
     */
    bool removeSkill(const QString &skillName);

    /**
     * @brief 检查技能是否存在
     * @param skillName 技能名称
     * @return 存在返回 true，否则返回 false
     */
    bool hasSkill(const QString &skillName) const;

    /**
     * @brief 设置文件系统监视器
     *
     * 监控技能目录的文件变化，自动触发重新加载
     */
    void setupFileWatcher();

Q_SIGNALS:
    /**
     * @brief 技能列表更新信号
     *
     * 当技能文件发生变化导致技能列表更新时发出
     */
    void skillsUpdated();

private:
    /**
     * @brief 获取技能存放的目录路径列表
     * @return 技能目录路径列表
     */
    static QStringList skillPaths();

    /**
     * @brief 从指定目录加载技能
     * @param directory 技能目录路径
     */
    void loadSkillsFromDirectory(const QString &directory);

    /**
     * @brief 解析技能文件的front matter元数据
     * @param frontMatter front matter字符串
     * @param skill 输出参数，将被填充解析结果
     */
    void parseSkillFrontMatter(const QString &content, SkillInfo &skill);

    /**
     * @brief 解析YAML格式的front matter
     * @param frontMatter YAML front matter字符串
     * @param skill 输出参数，将被填充解析结果
     */
    void parseYamlFrontMatter(const QString &frontMatter, SkillInfo &skill);

    /**
     * @brief 校验 SKILL.md 的内容：name 字段非空，且不与其他来源技能冲突
     * @param actualPath  技能目录路径
     * @param errorMsg    可选输出：失败时的错误描述
     * @param outName     可选输出：校验通过时填入 SKILL.md 中解析出的技能名称
     * @return 校验通过返回 true
     */
    bool validateSkillMd(const QString &actualPath, QString *errorMsg, QString *outName = nullptr);

    /**
     * @brief 递归拷贝目录
     * @param sourcePath 源目录路径
     * @param targetPath 目标目录路径
     * @return 拷贝是否成功
     */
    static bool copyDirectory(const QString &sourcePath, const QString &targetPath);

    /**
     * @brief 将文件或目录移动到回收站
     * @param path 文件或目录路径
     * @return 移动是否成功
     */
    static bool moveToTrash(const QString &path);

private:
    QMap<QString, SkillInfo> m_skills; /* 已加载的技能信息映射，键为技能名称 */
    QFileSystemWatcher m_watcher; /* 文件系统监视器，用于监控技能文件变化 */
};

}   // namespace uos_ai

#endif   // SKILLS_MANAGER_H
