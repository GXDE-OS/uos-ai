#include "skillsmanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>

#include <yaml-cpp/yaml.h>

// Qt5/6 compatibility for SkipEmptyParts
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
#    define SKIP_EMPTY_PARTS QString::SkipEmptyParts
#else
#    define SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#endif

Q_LOGGING_CATEGORY(skillsManager, "uos.ai.skills")

namespace uos_ai {

namespace {
/* 技能状态配置文件路径 */
static const QString &skillsConfigPath()
{
    static const QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/deepin/uos-ai-assistant/skills.conf";
    return path;
}

/* 文件 + 目录数量阈值，防止拷贝超大目录 */
static const int MAX_SKILL_ENTRIES = 200;

/* 文件夹总大小阈值：10MB */
static const qint64 MAX_SKILL_DIR_SIZE = 10 * 1024 * 1024;

/* 初始化默认技能路径配置 */
static void initializeDefaultPaths()
{
    QString configPath = skillsConfigPath();

    /* 如果配置文件已存在，无需初始化 */
    if (QFile::exists(configPath)) {
        return;
    }

    /* 确保配置目录存在 */
    QFileInfo fileInfo(configPath);
    QString configDir = fileInfo.path();
    QDir dir;
    if (!dir.exists(configDir)) {
        dir.mkpath(configDir);
    }

    /* 写入默认配置 */
    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup("paths");
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    settings.setValue("path_claude", homePath + "/.claude/skills");
    settings.setValue("path_agents", homePath + "/.agents/skills");
    settings.endGroup();
    settings.sync();

    qCDebug(skillsManager) << "Initialized default skill paths configuration:" << configPath;
}

/* 默认预装技能路径（对所有用户有效） */
static const QString &defaultSkillsPath()
{
#ifdef UOS_AI_SKILLS_DEFAULT
    static const QString path = QStringLiteral(UOS_AI_SKILLS_DEFAULT);
#else
    /* 编译时未定义，使用默认路径 */
    static const QString path = QStringLiteral("/usr/lib/uos-ai-assistant/skills-default");
#endif
    return path;
}

/* 从配置文件读取自定义技能路径列表 */
static QStringList readCustomSkillPaths()
{
    initializeDefaultPaths();

    QSettings settings(skillsConfigPath(), QSettings::IniFormat);
    /* 读取 paths 节下的 path 键列表 */
    settings.beginGroup("paths");
    QStringList customPaths;
    for (const QString &key : settings.childKeys()) {
        if (key.startsWith("path")) {
            QString path = settings.value(key).toString();
            if (!path.isEmpty()) {
                customPaths.append(path);
            }
        }
    }
    settings.endGroup();
    return customPaths;
}

/**
 * @brief 两级排序：来源 + 技能名称
 * @return 排序后的技能列表
 *
 * 排序规则：
 * 1. 一级排序：uos-ai 放在第一位，其余按来源名称自然顺序
 * 2. 二级排序：在同一来源内，按技能名称排序
 */
static QList<SkillInfo> sortSkills(const QList<SkillInfo> &skills)
{
    QList<SkillInfo> result = skills;

    std::sort(result.begin(), result.end(), [](const SkillInfo &a, const SkillInfo &b) {
        /* 一级排序：uos-ai 放在第一位，其余按来源名自然顺序 */
        bool aIsUosAi = (a.source == "uos-ai");
        bool bIsUosAi = (b.source == "uos-ai");

        if (aIsUosAi && !bIsUosAi) {
            return true;
        }
        if (!aIsUosAi && bIsUosAi) {
            return false;
        }
        if (a.source != b.source) {
            /* 非uos-ai来源，按来源名自然顺序 */
            return a.source < b.source;
        }

        /* 二级排序：同一来源内，按技能名称排序 */
        return a.name < b.name;
    });

    return result;
}

/**
 * @brief 检查路径是否为支持的压缩包格式
 */
static bool isArchiveFile(const QString &path)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    return mime.inherits("application/zip") || mime.inherits("application/x-tar");
}

/**
 * @brief 调用系统命令解压压缩包到指定目录
 * @return true 解压成功，false 超时或命令失败
 */
static bool extractArchive(const QString &archivePath, const QString &targetDir)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(archivePath, QMimeDatabase::MatchContent);

    QProcess process;
    if (mime.inherits("application/zip")) {
        process.start("unzip", QStringList() << "-q" << archivePath << "-d" << targetDir);
    } else {
        process.start("tar", QStringList() << "-xf" << archivePath << "-C" << targetDir);
    }
    if (!process.waitForFinished(30000)) {
        qWarning() << "Archive extraction timed out:" << archivePath;
        return false;
    }
    if (process.exitCode() != 0) {
        qWarning() << "Archive extraction failed:" << process.readAllStandardError();
        return false;
    }
    return true;
}

/**
 * @brief 在解压根目录中定位技能目录
 *
 * 优先深度1：寻找含 SKILL.md 的子目录（zip -r skill.zip skill/ 形式）
 * 降级深度0：若 SKILL.md 直接在解压根目录（zip 打包内容而非目录）
 *
 * @return 技能目录的绝对路径，未找到则返回空字符串
 */
static QString findSkillDirInExtracted(const QString &extractedRoot)
{
    /* 深度1：查找含 SKILL.md 的子目录 */
    QDir rootDir(extractedRoot);
    QFileInfoList subdirs = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subdirInfo : subdirs) {
        QString skillMdPath = subdirInfo.absoluteFilePath() + "/SKILL.md";
        if (QFile::exists(skillMdPath)) {
            return subdirInfo.absoluteFilePath();
        }
    }

    /* 深度0：SKILL.md 直接在解压根目录 */
    if (QFile::exists(extractedRoot + "/SKILL.md")) {
        return extractedRoot;
    }

    return QString();
}

/**
 * @brief 解析结果：持有技能源目录、最终技能名称以及（若有）临时解压目录
 *
 * isValid() 为 false 时表示解析失败。
 */
struct SkillSource {
    QString actualPath;                           /* 技能目录的绝对路径 */
    QString skillName;                            /* 最终技能名称（目录名或压缩包名） */
    std::unique_ptr<QTemporaryDir> tempDirHandle; /* 临时解压目录生命周期管理（非压缩包场景为空） */

    SkillSource() = default;
    SkillSource(const SkillSource &) = delete;
    SkillSource &operator=(const SkillSource &) = delete;
    SkillSource(SkillSource &&) = default;
    SkillSource &operator=(SkillSource &&) = default;

    bool isValid() const { return !actualPath.isEmpty(); }
};

/**
 * @brief 从压缩包解析技能源
 *
 * 解压到临时目录，定位含 SKILL.md 的技能目录，推导技能名称。
 */
static SkillSource resolveFromArchive(const QString &path, QString *errorMsg)
{
    auto setError = [&](const QString &msg) {
        if (errorMsg)
            *errorMsg = msg;
    };

    SkillSource src;

    QFileInfo archiveInfo(path);
    if (!archiveInfo.exists() || !archiveInfo.isFile()) {
        qCWarning(skillsManager) << "Archive file does not exist:" << path;
        setError(SkillsManager::tr("The archive file does not exist."));
        return src;
    }

    src.tempDirHandle = std::make_unique<QTemporaryDir>();
    if (!src.tempDirHandle->isValid()) {
        qCWarning(skillsManager) << "Failed to create temp dir:" << src.tempDirHandle->errorString();
        setError(SkillsManager::tr("Failed to create temporary directory for extraction."));
        return src;
    }

    const QString tempPath = src.tempDirHandle->path();

    if (!extractArchive(path, tempPath)) {
        qCWarning(skillsManager) << "Failed to extract archive:" << path;
        setError(SkillsManager::tr("Failed to extract the archive. The file may be corrupted or the format is not supported."));
        return src;
    }

    QString foundDir = findSkillDirInExtracted(tempPath);
    if (foundDir.isEmpty()) {
        qCWarning(skillsManager) << "No SKILL.md found in archive:" << path;
        setError(SkillsManager::tr("No SKILL.md file found in the archive."));
        return src;
    }

    src.actualPath = foundDir;
    if (foundDir == tempPath) {
        /* SKILL.md 直接在解压根目录：用压缩包文件名作技能名 */
        src.skillName = archiveInfo.baseName();
        if (src.skillName.endsWith(".tar", Qt::CaseInsensitive))
            src.skillName.chop(4);
    } else {
        src.skillName = QDir(foundDir).dirName();
    }

    return src;
}

/**
 * @brief 从普通文件（SKILL.md）或目录解析技能源
 */
static SkillSource resolveFromPath(const QString &path, QString *errorMsg)
{
    SkillSource src;
    QFileInfo fi(path);

    if (fi.isFile()) {
        src.actualPath = fi.path();
    } else if (fi.isDir()) {
        src.actualPath = fi.filePath();
    } else {
        qCWarning(skillsManager) << "Invalid path type:" << path;
        if (errorMsg)
            *errorMsg = SkillsManager::tr("The specified path is invalid.");
        return src;
    }

    src.skillName = QDir(src.actualPath).dirName();
    return src;
}

/**
 * @brief 将用户提供的路径解析为 SkillSource
 */
static SkillSource resolveSkillPath(const QString &path, QString *errorMsg)
{
    if (isArchiveFile(path))
        return resolveFromArchive(path, errorMsg);
    return resolveFromPath(path, errorMsg);
}

} // end anonymous namespace

/* ========== SkillInfo 实现 ========== */

void SkillInfo::loadDetails() const
{
    m_detailsLoaded = true;

    if (path.isEmpty()) {
        qCWarning(skillsManager) << "Cannot load details: path is empty";
        m_details = QString();
        return;
    }

    /* 查找技能文件 */
    QString skillFilePath = path + "/SKILL.md";
    QFile skillFile(skillFilePath);

    if (!skillFile.exists()) {
        /* 尝试查找其他 .md 文件 */
        QDir dir(path);
        QStringList mdFiles = dir.entryList(QStringList() << "*.md", QDir::Files);
        if (!mdFiles.isEmpty()) {
            skillFilePath = path + "/" + mdFiles.first();
            skillFile.setFileName(skillFilePath);
        } else {
            qCWarning(skillsManager) << "No skill file found in:" << path;
            m_details = QString();
            return;
        }
    }

    if (!skillFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(skillsManager) << "Failed to open skill file:" << skillFilePath;
        m_details = QString();
        return;
    }

    QString content;
    QTextStream in(&skillFile);
    content = in.readAll();
    skillFile.close();

    /* 提取 details 部分（去除 front matter） */
    if (content.startsWith("---")) {
        int endPos = content.indexOf("---", 3);
        if (endPos > 0) {
            m_details = content.mid(endPos + 3).trimmed();
            return;
        }
    }

    /* 没有 front matter，返回全部内容 */
    m_details = content.trimmed();
}

void SkillInfo::loadFiles() const
{
    m_filesLoaded = true;

    if (path.isEmpty()) {
        qCWarning(skillsManager) << "Cannot load files: path is empty";
        m_files.clear();
        return;
    }

    QDir dir(path);
    if (!dir.exists()) {
        qCWarning(skillsManager) << "Skill directory does not exist:" << path;
        m_files.clear();
        return;
    }

    /* 递归遍历目录，收集文件列表 */
    m_files.clear();
    collectFiles(dir);
}

void SkillInfo::collectFiles(const QDir &dir) const
{
    /* 先处理文件 */
    QFileInfoList fileInfos = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileInfo : fileInfos) {
        QString fileName = fileInfo.fileName();
        /* 排除 LICENSE 系列文件（不区分大小写）和 README 系列文件（不区分大小写） */
        if (fileName.startsWith("LICENSE", Qt::CaseInsensitive) ||
            fileName.startsWith("README", Qt::CaseInsensitive) ||
            fileName.compare("SKILL.md", Qt::CaseInsensitive) == 0) {
            continue;
        }
        /* 添加完整绝对路径 */
        m_files.append(fileInfo.absoluteFilePath());
    }

    /* 递归处理子目录 */
    QFileInfoList dirInfos = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &dirInfo : dirInfos) {
        QDir subdir(dirInfo.absoluteFilePath());
        collectFiles(subdir);
    }
}

/* ========== SkillsManager 实现 ========== */

SkillsManager::SkillsManager(QObject *parent)
    : QObject(parent)
{
    reloadSkills();
    setupFileWatcher();
}

SkillsManager::~SkillsManager() {}

QString SkillsManager::prompt() const
{
    QString result;
    QTextStream stream(&result);

    QList<SkillInfo> enabled = enabledSkills();

    stream << "# AGENTS\n\n";
    stream << "<skills_system priority=\"1\">\n\n";
    stream << "## Available Skills\n\n";
    stream << "<!-- SKILLS_TABLE_START -->\n";
    stream << "<usage>\n";
    stream << "When users ask you to perform tasks, check if any of the available "
              "skills below can help complete the task more effectively. Skills "
              "provide specialized capabilities and domain knowledge.\n\n";
    stream << "How to use skills:\n";
    stream << "- Review the skill descriptions in <available_skills> below\n";
    stream << "- Check if a skill matches the user's request or task context\n";
    stream << "- Apply the skill's instructions and guidelines to complete the "
              "task\n";
    stream << "- Each skill contains detailed instructions on usage and best "
              "practices\n\n";
    stream << "Usage notes:\n";
    stream << "- Only use skills listed in <available_skills> below\n";
    stream << "- Skills are already loaded in your context, no invocation "
              "needed\n";
    stream << "- Apply skill knowledge directly when relevant to the task\n";
    stream << "</usage>\n\n";

    stream << "<available_skills>\n\n";

    if (enabled.isEmpty()) {
        stream << "<!-- No skills are currently enabled -->\n";
    } else {
        for (const auto &skill : enabled) {
            stream << "<skill>\n";
            stream << "<name>" << skill.name << "</name>\n";

            if (!skill.description.isEmpty()) {
                stream << "<description>" << skill.description << "</description>\n";
            }

            //   if (!skill.details.isEmpty()) {
            //     stream << "<details>\n";
            //     stream << skill.details << "\n";
            //     stream << "</details>\n";
            //   }

            stream << "</skill>\n\n";
        }
    }

    stream << "</available_skills>\n";
    stream << "<!-- SKILLS_TABLE_END -->\n\n";
    stream << "</skills_system>\n";

    return result;
}

SkillInfo SkillsManager::readSkill(const QString &skillName) const
{
    return m_skills.value(skillName, {});
}

QList<SkillInfo> SkillsManager::skills() const
{
    return sortSkills(m_skills.values());
}

QList<SkillInfo> SkillsManager::enabledSkills() const
{
    QList<SkillInfo> result;
    for (const auto &skill : m_skills) {
        if (skill.enabled) {
            result.append(skill);
        }
    }
    return sortSkills(result);
}

bool SkillsManager::setSkillEnabled(const QString &skillName, bool enabled)
{
    if (!m_skills.contains(skillName)) {
        return false;
    }

    if (m_skills[skillName].enabled == enabled) {
        /* 状态未改变，无需保存 */
        return true;
    }

    m_skills[skillName].enabled = enabled;

    /* 保存到统一配置文件 */
    QSettings settings(skillsConfigPath(), QSettings::IniFormat);
    /* 键名格式: source/skillName/enabled */
    QString source = m_skills[skillName].source;
    settings.setValue(source + "/" + skillName + "/enabled", enabled);
    settings.sync(); /* 立即写入磁盘 */

    Q_EMIT skillsUpdated();
    return true;
}

// todo 异步加载
void SkillsManager::reloadSkills()
{
    m_skills.clear();

    QStringList paths = skillPaths();
    for (const QString &path : paths) {
        loadSkillsFromDirectory(path);
    }

    Q_EMIT skillsUpdated();
}

bool SkillsManager::hasSkill(const QString &skillName) const
{
    return m_skills.contains(skillName);
}

QStringList SkillsManager::skillPaths()
{
    QStringList paths;

    /* 去重：规范化后保序去重，确保 uos-ai 路径始终最先加载 */
    QStringList result;
    QSet<QString> seen;

    /* 优先级1：用户自定义路径（优先级最高，可覆盖其他来源） */
    QString userPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.uos-ai/skills";
    QString cleanUserPath = QDir::cleanPath(userPath);
    if (!seen.contains(cleanUserPath)) {
        seen.insert(cleanUserPath);
        result.append(cleanUserPath);
    }

    /* 优先级2：配置文件的自定义路径 */
    QStringList customPaths = readCustomSkillPaths();
    for (const QString &path : customPaths) {
        QString clean = QDir::cleanPath(path);
        if (!seen.contains(clean)) {
            seen.insert(clean);
            result.append(clean);
        }
    }

    /* 优先级3：默认预装路径（优先级最低，被所有用户路径覆盖） */
    QString defaultPath = defaultSkillsPath();
    QString cleanDefaultPath = QDir::cleanPath(defaultPath);
    if (!seen.contains(cleanDefaultPath)) {
        seen.insert(cleanDefaultPath);
        result.append(cleanDefaultPath);
    }

    return result;
}

void SkillsManager::loadSkillsFromDirectory(const QString &directory)
{
    QDir baseDir(directory);
    if (!baseDir.exists()) {
        /* Only uos-ai skills directory should be auto-created */
        if (directory.endsWith("/.uos-ai/skills")) {
            baseDir.mkpath(".");
            qCWarning(skillsManager) << "Skills directory does not exist, created:" << directory;
        } else {
            qCWarning(skillsManager) << "Skills directory does not exist:" << directory;
        }
        return;
    }

    /* 从目录路径中提取来源 (uos-ai, claude, builtin, etc.) */
    QString source = "unknown";

    /* 检查是否是默认预装路径，优先标识为 builtin */
    QString cleanDir = QDir::cleanPath(directory);
    if (cleanDir == QDir::cleanPath(defaultSkillsPath())) {
        source = "builtin";
    } else {
        /* 普通路径逻辑：从目录结构提取来源 */
        QDir pathDir(directory);
        QString dirName = pathDir.dirName();   // "skills"
        if (dirName == "skills") {
            pathDir.cdUp();   // 上一级目录
            QString parentName = pathDir.dirName();   // ".claude"、".uos-ai" 或自定义路径的父目录名
            if (parentName.startsWith(".")) {
                source = parentName.mid(1);   // 去掉前面的点号，如 "claude"、"uos-ai"
            } else {
                source = parentName;   /* 直接使用父目录名 */
            }
        }

        /* 防撞：防止用户自定义路径被误识别为 builtin 或 uos-ai */
        QString uosAiSkillsPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.uos-ai/skills");
        if ((source == "builtin" && cleanDir != QDir::cleanPath(defaultSkillsPath())) ||
            (source == "uos-ai" && cleanDir != uosAiSkillsPath)) {
            /* 不是真正的系统路径，重命名为 xxx-custom */
            source = source + "-custom";
        }
    }

    /* 遍历技能目录下的子目录，每个子目录代表一个技能 */
    QFileInfoList subdirs =
            baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QFileInfo &subdirInfo : subdirs) {
        QString skillDir = subdirInfo.absoluteFilePath();
        QString skillName = subdirInfo.fileName();

        /* 查找子目录中的 SKILL.md 文件 */
        QFile skillFile(skillDir + "/SKILL.md");
        if (!skillFile.exists()) {
            /* 如果没有 SKILL.md，尝试查找任意 .md 文件 */
            QDir dir(skillDir);
            QStringList mdFiles = dir.entryList(QStringList() << "*.md", QDir::Files);
            if (!mdFiles.isEmpty()) {
                skillFile.setFileName(skillDir + "/" + mdFiles.first());
            } else {
                qCWarning(skillsManager) << "No skill file found in directory:" << skillDir;
                continue;
            }
        }

        if (!skillFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(skillsManager) << "Failed to open skill file:" << skillFile.fileName();
            continue;
        }

        QString content;
        QTextStream in(&skillFile);
        content = in.readAll().trimmed();
        skillFile.close();

        /* 先解析 SKILL.md 获取真实的 skill name */
        SkillInfo skill;
        skill.path = skillDir; /* 技能所在目录 */
        skill.source = source; /* 从目录路径提取的来源 */

        parseSkillFrontMatter(content, skill);

        /* name 字段缺失，跳过非标准 skill */
        if (skill.name.isEmpty()) {
            qCWarning(skillsManager) << "Skill name not defined in front matter, skipping:" << skillDir;
            continue;
        }

        /* 如果已存在同名技能（来自优先级更高的路径），跳过 */
        if (m_skills.contains(skill.name)) {
            qCDebug(skillsManager) << "Skill already exists, skipping:" << skill.name
                                   << "from" << source;
            continue;
        }

        /* 从统一配置文件读取启用状态，默认启用 */
        QSettings settings(skillsConfigPath(), QSettings::IniFormat);
        skill.enabled = settings.value(source + "/" + skill.name + "/enabled", true).toBool();

        m_skills[skill.name] = skill;
    }
}

void SkillsManager::setupFileWatcher()
{
    QStringList paths = skillPaths();
    for (const QString &path : paths) {
        QDir dir(path);
        if (dir.exists()) {
            m_watcher.removePath(path);
            m_watcher.addPath(path);
        }
    }

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &SkillsManager::reloadSkills);
}

void SkillsManager::parseSkillFrontMatter(const QString &content,
                                          SkillInfo &skill)
{
    if (!content.startsWith("---")) {
        /* 没有 front matter，使用目录名作为 description */
        /* details 不再在此加载，改为按需读取 */
        skill.description = skill.name.isEmpty() ? "Unknown skill" : skill.name;
        return;
    }

    int endPos = content.indexOf("---", 3);
    if (endPos > 0) {
        /* 解析 YAML front matter */
        QString frontMatter = content.mid(3, endPos - 3).trimmed();
        /* details 不再在此加载到内存 */

        parseYamlFrontMatter(frontMatter, skill);
    } else {
        /* 只有开头 --- 没有结束，按无 front matter 处理 */
        skill.description = skill.name.isEmpty() ? "Unknown skill" : skill.name;
    }

    /* 如果 description 为空，使用 name */
    if (skill.description.isEmpty()) {
        skill.description = skill.name;
    }
}

void SkillsManager::parseYamlFrontMatter(const QString &frontMatter,
                                         SkillInfo &skill)
{
    try {
        YAML::Node node = YAML::Load(frontMatter.toStdString());

        /* 解析 name 字段 */
        if (node["name"]) {
            skill.name = QString::fromStdString(node["name"].as<std::string>());
        }

        /* 解析 description 字段 */
        if (node["description"]) {
            skill.description = QString::fromStdString(node["description"].as<std::string>());
        }
    } catch (const YAML::Exception &e) {
        qCWarning(skillsManager) << "YAML parse error:" << e.what();
    }
}

/**
 * @brief 检查路径是否在危险的黑名单目录中
 */
static bool isDangerousPath(const QString &path, const QString &homePath)
{
    QString normalizedPath = QDir::cleanPath(path);

    /* 根目录 */
    if (normalizedPath == "/" || normalizedPath.isEmpty()) {
        return true;
    }

    /* 系统关键目录 */
    const QStringList systemDirs = {"/etc", "/usr", "/var", "/tmp", "/sys", "/proc", "/dev", "/run", "/boot", "/home"};
    for (const QString &sysDir : systemDirs) {
        if (normalizedPath == sysDir) {
            return true;
        }
    }

    /* 用户主目录本身 */
    if (normalizedPath == homePath) {
        return true;
    }

    /* ~/.uos-ai 及其子目录（防止循环嵌套） */
    if (normalizedPath.startsWith(homePath + "/.uos-ai")) {
        return true;
    }

    return false;
}

/**
 * @brief 统计目录中的文件和子目录数量（限制递归深度）
 */
static int countDirectoryEntries(const QString &dirPath)
{
    QDir dir(dirPath);
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);

    int count = 0;
    for (const QFileInfo &entry : entries) {
        count++;
        if (entry.isDir()) {
            /* 递归统计子目录（限制递归深度） */
            count += countDirectoryEntries(entry.absoluteFilePath());
            /* 如果已经超过阈值，提前返回 */
            if (count > MAX_SKILL_ENTRIES) {
                return count;
            }
        }
    }
    return count;
}

/**
 * @brief 统计目录的总大小（迭代遍历所有子目录）
 * @return 目录总大小（字节）
 */
static qint64 calculateDirectorySize(const QString &dirPath)
{
    qint64 totalSize = 0;
    QStringList dirsToCheck;
    dirsToCheck << dirPath;

    while (!dirsToCheck.isEmpty()) {
        QString currentPath = dirsToCheck.takeLast();
        QDir currentDir(currentPath);
        QFileInfoList entries = currentDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);

        for (const QFileInfo &entry : entries) {
            if (entry.isFile()) {
                totalSize += entry.size();
            } else if (entry.isDir()) {
                /* 将子目录加入待遍历列表 */
                dirsToCheck << entry.absoluteFilePath();
            }
        }
    }
    return totalSize;
}

bool SkillsManager::validateSkillMd(const QString &actualPath, QString *errorMsg, QString *outName)
{
    auto setError = [&](const QString &msg) {
        if (errorMsg)
            *errorMsg = msg;
    };

    QFile skillFile(actualPath + "/SKILL.md");
    if (!skillFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(skillsManager) << "Failed to open SKILL.md for validation:" << actualPath;
        setError(tr("Failed to read SKILL.md."));
        return false;
    }
    QString content = QTextStream(&skillFile).readAll().trimmed();
    skillFile.close();

    SkillInfo tempSkill;
    parseSkillFrontMatter(content, tempSkill);
    if (tempSkill.name.isEmpty()) {
        qCWarning(skillsManager) << "SKILL.md is missing required 'name' field in:" << actualPath;
        setError(tr("SKILL.md is missing the required 'name' field."));
        return false;
    }

    if (outName)
        *outName = tempSkill.name;

    return true;
}

bool SkillsManager::addSkill(const QString &path, QString *errorMsg, QString *outSkillName)
{
    auto setError = [&](const QString &msg) {
        if (errorMsg)
            *errorMsg = msg;
    };

    /* 解析路径 → SkillSource */
    SkillSource src = resolveSkillPath(path, errorMsg);
    if (!src.isValid())
        return false;

    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir sourceDir(src.actualPath);

    /* 验证技能目录存在 */
    if (!sourceDir.exists()) {
        qCWarning(skillsManager) << "Skill folder does not exist:" << src.actualPath;
        setError(tr("The skill directory does not exist."));
        return false;
    }

    /* 安全检查 1：路径黑名单检查 */
    if (isDangerousPath(QDir::cleanPath(src.actualPath), homePath)) {
        qCWarning(skillsManager) << "Refused to add skill from dangerous path:" << src.actualPath;
        setError(tr("The skill cannot be imported from this path."));
        return false;
    }

    /* 校验 SKILL.md 文件必须存在 */
    if (!sourceDir.exists("SKILL.md")) {
        qCWarning(skillsManager) << "SKILL.md file not found in:" << src.actualPath;
        setError(tr("No SKILL.md file found in the skill directory."));
        return false;
    }

    /* 校验 SKILL.md 内容与跨来源冲突 */
    if (!validateSkillMd(src.actualPath, errorMsg, outSkillName))
        return false;

    /* 安全检查 2：目录内容预检（防止拷贝超大目录） */
    int entryCount = countDirectoryEntries(src.actualPath);
    if (entryCount > MAX_SKILL_ENTRIES) {
        qCWarning(skillsManager) << "Skill directory too large (" << entryCount << " entries), max allowed:" << MAX_SKILL_ENTRIES;
        setError(tr("The skill contains too many files (maximum %1 allowed).").arg(MAX_SKILL_ENTRIES));
        return false;
    }

    /* 安全检查 3：检查文件夹总大小（>=10MB） */
    qint64 dirSize = calculateDirectorySize(src.actualPath);
    if (dirSize > MAX_SKILL_DIR_SIZE) {
        qCWarning(skillsManager) << "Cannot add skill: directory size too large (" << dirSize / (1024.0 * 1024.0) << " MB), max allowed is 10 MB";
        setError(tr("The skill size exceeds the limit (maximum 10 MB allowed)."));
        return false;
    }

    if (src.skillName.isEmpty()) {
        qCWarning(skillsManager) << "Invalid skill folder name";
        setError(tr("The skill folder name is invalid."));
        return false;
    }

    /* 获取目标路径: ~/.uos-ai/skills/<skillName>/ */
    QString targetDirPath = homePath + "/.uos-ai/skills/" + src.skillName;
    QDir targetDir(targetDirPath);

    /* 如果目标文件夹已存在，移动到回收站以便覆盖 */
    if (targetDir.exists()) {
        if (!moveToTrash(targetDirPath)) {
            qCWarning(skillsManager) << "Failed to move existing skill folder to trash:" << targetDirPath;
            setError(tr("Failed to overwrite the existing skill."));
            return false;
        }
        qCDebug(skillsManager) << "Moved existing skill folder to trash for overwrite:" << targetDirPath;
    }

    /* 创建目标父目录（确保 ~/.uos-ai/skills/ 存在） */
    QDir parentDir = targetDir;
    if (!parentDir.cdUp() || !parentDir.exists()) {
        if (!parentDir.mkpath(".")) {
            qCWarning(skillsManager) << "Failed to create skills directory:" << parentDir.path();
            setError(tr("Failed to create the skills directory."));
            return false;
        }
    }

    /* 拷贝整个文件夹 */
    if (!copyDirectory(sourceDir.path(), targetDirPath)) {
        qCWarning(skillsManager) << "Failed to copy skill directory from" << sourceDir.path() << "to" << targetDirPath;
        setError(tr("Failed to copy the skill files."));
        return false;
    }

    qCDebug(skillsManager) << "Skill added successfully:" << src.skillName;
    return true;
}

bool SkillsManager::removeSkill(const QString &skillName)
{
    /* 检查技能是否存在于 m_skills 中 */
    if (!m_skills.contains(skillName)) {
        qCWarning(skillsManager) << "Skill not found:" << skillName;
        return false;
    }

    const SkillInfo &skill = m_skills[skillName];

    /* 仅允许删除来自 uos-ai 路径的技能 */
    QString uosAiSkillsPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.uos-ai/skills";
    if (!skill.path.startsWith(uosAiSkillsPath)) {
        qCWarning(skillsManager) << "Cannot remove skill from non-uos-ai path:" << skill.path;
        return false;
    }

    /* 移动到回收站 */
    if (!moveToTrash(skill.path)) {
        qCWarning(skillsManager) << "Failed to move skill to trash:" << skill.path;
        return false;
    }

    qCDebug(skillsManager) << "Skill moved to trash:" << skillName;
    return true;
}

bool SkillsManager::copyDirectory(const QString &sourcePath, const QString &targetPath)
{
    QDir sourceDir(sourcePath);
    QDir targetDir;

    /* 创建目标目录 */
    if (!targetDir.mkpath(targetPath)) {
        return false;
    }

    /* 遍历源目录中的所有文件和子目录 */
    QFileInfoList entries = sourceDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (const QFileInfo &entry : entries) {
        QString srcPath = entry.absoluteFilePath();
        QString dstPath = targetPath + "/" + entry.fileName();

        if (entry.isDir()) {
            /* 递归拷贝子目录 */
            if (!copyDirectory(srcPath, dstPath)) {
                return false;
            }
        } else {
            /* 拷贝文件 */
            if (!QFile::copy(srcPath, dstPath)) {
                return false;
            }
        }
    }
    return true;
}

bool SkillsManager::moveToTrash(const QString &path)
{
    /* Qt 5.14+ 使用 API，否则使用 gio trash 命令 */
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (!QFile::moveToTrash(path)) {
        qCWarning(skillsManager) << "Failed to move to trash using Qt API:" << path;
        return false;
    }
#else
    /* 使用 gio trash 命令移动到回收站 */
    QProcess process;
    process.start("gio", QStringList() << "trash" << path);
    if (!process.waitForFinished(5000)) {
        qCWarning(skillsManager) << "gio trash command timeout";
        return false;
    }
    if (process.exitCode() != 0) {
        qCWarning(skillsManager) << "gio trash command failed:" << process.readAllStandardError();
        return false;
    }
#endif
    return true;
}

}   // namespace uos_ai
