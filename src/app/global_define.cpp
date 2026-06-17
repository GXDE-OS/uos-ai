#include "global_define.h"
#include "global_key_define.h"

#include <QString>
#include <QUuid>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QVersionNumber>
#include <QLoggingCategory>

using namespace uos_ai;

static const QHash<QString, ContentType> contentTypeMap = {
    {STR_KEY_TEXT, CntText},
    {STR_KEY_IMAGE, CntImage},
    {STR_KEY_FILE, CntFile},
    {STR_KEY_TOOL, CntTool},
    {STR_KEY_INSTRUCTION, CntInstruction},
    {STR_KEY_THINKING, CntReasoning},
    {STR_KEY_AGENT_STEP, CntAgentStep},
    {STR_KEY_OUTLINE, CntOutline},
    {STR_KEY_DOC_CARD, CntDocCard},
    {STR_KEY_COMMAND_CARD, CntCommandCard},
    {STR_KEY_GUESS_YOU_WANT, CntGuessYouWant},
    {STR_KEY_ERROR, CntError},
    {STR_KEY_WEB_SEARCH, CntWebSearch},
    {STR_KEY_INTERACTIVE_COMPONENTPS, CntIComps}
};

QString GlobalUtil::contentTypeToString(ContentType type)
{
    return contentTypeMap.key(type);
}

ContentType GlobalUtil::contentTypeFromString(const QString &str)
{
    return contentTypeMap.value(str);
}

QString GlobalUtil::generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString GlobalUtil::generateMsId()
{
    return QString("%0_%1")
            .arg(QDateTime::currentDateTime().toMSecsSinceEpoch())
            .arg(rand() % 9000 + 1000);
}

// 获取系统版本号
static QString getOsVersion()
{
    QFile file("/etc/deepin-version");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("Version=")) {
            // 提取版本号，如 "Version=20" -> "20"
            QString version = line.mid(8).trimmed();
            file.close();
            return version;
        }
    }
    file.close();
    return QString();
}

// 获取 deepin-icon-theme 包版本
static QString getDeepinIconThemeVersion()
{
    QFile file("/var/lib/dpkg/status");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    bool foundPackage = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        if (line == "Package: deepin-icon-theme") {
            foundPackage = true;
        } else if (foundPackage && line.startsWith("Version:")) {
            // 提取版本号，如 "Version: 2024.12.05-1" -> "2024.12.05-1"
            QString version = line.mid(9).trimmed();
            file.close();
            return version;
        } else if (foundPackage && line.startsWith("Package:")) {
            // 遇到下一个包，说明没找到
            break;
        }
    }
    file.close();
    return QString();
}

// 比较版本号，返回 true 如果 v1 < v2
static bool versionLessThan(const QString &v1, const QString &v2)
{
    // 移除 debian 修订号后缀（如 "2024.12.05-1" -> "2024.12.05"）
    QString cleanV1 = v1.section('-', 0, 0);
    QString cleanV2 = v2.section('-', 0, 0);
    
    QVersionNumber ver1 = QVersionNumber::fromString(cleanV1);
    QVersionNumber ver2 = QVersionNumber::fromString(cleanV2);
    
    return ver1 < ver2;
}

// 应用图标名称（运行时初始化）
static const char* g_applicationIconName = "UosAiAssistant";

const char* uos_ai::getApplicationIconName()
{
    return g_applicationIconName;
}

void uos_ai::initApplicationIconName()
{
    // 检查系统版本是否为 v25
    QString osVersion = getOsVersion();
    if (osVersion == "25") {
        // v25 系统使用新图标名
        g_applicationIconName = "UosAiAssistant";
        return;
    }
    
    // 非 v25 系统，检查 deepin-icon-theme 版本
    QString iconThemeVersion = getDeepinIconThemeVersion();
    if (iconThemeVersion.isEmpty()) {
        // 无法获取版本，使用旧图标名
        g_applicationIconName = "uos-ai-assistant";
        return;
    }
    
    // 如果版本 < 2024.12.05-1，使用旧图标名
    if (versionLessThan(iconThemeVersion, "2024.12.05")) {
        g_applicationIconName = "uos-ai-assistant";
        return;
    }
    
    g_applicationIconName = "UosAiAssistant";
}

