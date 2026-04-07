#include "chatbotagent.h"
#include "chatbotpaths.h"
#include "mcpclient.h"
#include "networkdefs.h"
#include "global_define.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QUuid>

// Bootstrap 引导内容（首次启动时注入用户消息，指导模型完成身份初始化）
static const QString kBootstrapGuidance = QStringLiteral(
R"([系统提示：这是你第一次上线，请先完成身份初始化]

_你刚醒来。该搞清楚自己是谁了。_

还没有记忆。这是全新的工作区，记忆文件在你创建之前不存在很正常。

## 对话

像这样开始：

> "嘿，我刚上线。我是谁？你是谁？"

然后一起搞清楚：

1. **你的名字** — 他们该怎么叫你？
2. **你的定位** — 你是什么？（AI 助手挺好，但也许你是更怪的东西）
3. **你的风格** — 正式？随意？调皮？温暖？怎样合适？
4. **其他** — 用户可以设置更多关于你的所有

如果用户没有直接回答你，就自己设定一些常规的答案吧，不要吓到用户。

## 知道自己是谁之后

把学到的写进对应文件（使用 update_profile 工具）：

- **PROFILE.md** — 「身份」section：你的名字、定位、风格；「用户资料」section：用户的名字、称呼、时区
- **SOUL.md** — 用户的价值观、期望你怎么做事、边界或偏好

## 完成后

确保以上内容都保存到文件后，用自然的方式开始正常对话。

---

_祝好运。活得精彩。_

---

以下是用户的消息：
)");

Q_DECLARE_LOGGING_CATEGORY(logAgent)

namespace uos_ai {
namespace chatbot {

ChatbotAgent::ChatbotAgent(QObject *parent)
    : MCPAgent(parent)
{
    m_name        = kChatbotAgentName;
    m_description = "A chatbot agent with MCP tools and skills support";

    m_systemPrompt = "You are UOSAI, a helpful assistant.";
}

QSharedPointer<LlmAgent> ChatbotAgent::create()
{
    return QSharedPointer<LlmAgent>(new ChatbotAgent());
}

QJsonObject ChatbotAgent::processRequest(const QJsonObject &question, const QJsonArray &history,
                                          const QVariantHash &params)
{
    // 每次请求前重建系统提示词
    m_platform           = params.value(QLatin1String(kChatbotPlatformParam)).toString();
    m_isBootstrapSession = !m_platform.isEmpty()
                           && !QFile::exists(ChatBotPaths::instance().profileMdFile(m_platform));

    m_systemPrompt = QStringLiteral(
        "你是一个 AI 助手。你的名字、定位和风格见下方「身份与配置」。\n\n"

        "## 🎯 主动记录 — 别等用户叫你记！\n\n"
        "对话中发现有价值的信息时，**主动调用 `update_profile` 工具**，再回答问题：\n\n"
        "- 用户提到的个人信息（名字、职业、偏好、习惯）→ 更新「用户资料」\n"
        "- 用户表达的喜好或不满 → 更新「用户资料」\n"
        "- 用户明确要求修改你的设定 → 更新「身份」或「行为准则」\n\n"
        "**原则：** 不要等用户说【记住这个】。信息对未来有价值就主动记。\n"
        "更新时在现有内容基础上修改，不要丢失已有信息，不要把本条指令写进去。\n"
        "工具调用在后台静默完成，无需告知用户。\n\n"

        "## 安全\n\n"
        "绝不泄露用户的私密信息给第三方。\n\n"

        "## 表情\n\n"
        "在 IM 平台上自然使用 emoji 回应，认可但不必回复时可以只用表情。别过度，每条消息最多一个。");

    // 注入 PROFILE.md 内容（身份与用户信息），若存在
    if (!m_platform.isEmpty()) {
        const QString profilePath = ChatBotPaths::instance().profileMdFile(m_platform);
        QFile profileFile(profilePath);
        if (profileFile.open(QIODevice::ReadOnly)) {
            const QString profileContent = QString::fromUtf8(profileFile.readAll()).trimmed();
            profileFile.close();
            if (!profileContent.isEmpty())
                m_systemPrompt += QStringLiteral("\n\n---\n\n## 身份与配置\n\n") + profileContent;
        }

        const QString soulPath = ChatBotPaths::instance().soulMdFile(m_platform);
        QFile soulFile(soulPath);
        if (soulFile.open(QIODevice::ReadOnly)) {
            const QString soulContent = QString::fromUtf8(soulFile.readAll()).trimmed();
            soulFile.close();
            if (!soulContent.isEmpty())
                m_systemPrompt += QStringLiteral("\n\n## 行为准则\n\n") + soulContent;
        }
    }

    // 注入历史压缩摘要
    const QString summary = params.value(QLatin1String(kChatbotHistorySummaryParam)).toString();
    if (!summary.isEmpty())
        m_systemPrompt += QStringLiteral("\n\n---\n\n## 历史对话记录\n\n") + summary;

    // Bootstrap Hook：首次上线时在用户消息中注入引导文本
    QJsonObject q = question;
    bootstrapHook(q);

    return MCPAgent::processRequest(q, history, params);
}

// ---------------------------------------------------------------------------
// MCP 通信接口：统一使用 kDefaultAgentName，因为 MCP 服务端只注册了该名字
// ---------------------------------------------------------------------------

bool ChatbotAgent::syncServers() const
{
    auto ret = syncCall<QPair<int, QJsonObject>>([this]() {
        return m_mcpClient->syncServers(kDefaultAgentName);
    });

    if (ret.first == 0) {
        auto details = ret.second.value("details").toObject();
        if (!details.isEmpty())
            qCInfo(logAgent) << "Synced servers" << details;
    } else {
        qCWarning(logAgent) << "Failed to sync servers:" << ret.second;
    }

    return ret.first == 0;
}

QPair<int, QString> ChatbotAgent::fetchTools(const QStringList &servers)
{
    m_tools = QJsonArray();

    // Bootstrap 阶段（PROFILE.md 不存在）：跳过所有 MCP / Skill 工具，
    // 只保留 update_profile，防止模型在自我介绍前调用 bash/文件工具检查环境。
    if (!m_isBootstrapSession) {
        // 从 MCP 服务端拉取工具，使用 kDefaultAgentName；servers 为空时跳过
        if (!servers.isEmpty()) {
            QPair<int, QJsonValue> srv = syncCall<QPair<int, QJsonValue>>([this, servers]() {
                return m_mcpClient->queryServers(kDefaultAgentName, servers);
            });

            if (srv.first == 0) {
                QStringList invaildSrv = servers;
                QStringList failedSrv;  // 首次查询报错的 server，稍后重试

                for (const QJsonValue &serverValue : srv.second.toArray()) {
                    if (!serverValue.isObject())
                        continue;
                    QJsonObject serverObj = serverValue.toObject();
                    const QString name   = serverObj.value("name").toString();

                    if (name.isEmpty() || !servers.contains(name))
                        continue;

                    invaildSrv.removeOne(name);

                    if (serverObj.contains("error")) {
                        qCWarning(logAgent) << "MCP server error:" << name
                                            << serverObj.value("error").toString()
                                            << "— will retry after sync";
                        failedSrv.append(name);
                        continue;
                    }

                    if (serverObj.contains("tools") && serverObj["tools"].isArray()) {
                        for (const QJsonValue &toolValue : serverObj["tools"].toArray())
                            m_tools.append(toolValue);
                    }
                }

                for (const QString &s : invaildSrv)
                    qCWarning(logAgent) << "MCP server not found:" << s;

                // P2：对首次失败的 server 执行 syncServers 后重试一次
                if (!failedSrv.isEmpty()) {
                    qCInfo(logAgent) << "Retrying failed MCP servers after sync:" << failedSrv;
                    syncServers();

                    QPair<int, QJsonValue> retry = syncCall<QPair<int, QJsonValue>>([this, failedSrv]() {
                        return m_mcpClient->queryServers(kDefaultAgentName, failedSrv);
                    });

                    if (retry.first == 0) {
                        for (const QJsonValue &serverValue : retry.second.toArray()) {
                            if (!serverValue.isObject())
                                continue;
                            QJsonObject serverObj = serverValue.toObject();
                            if (serverObj.contains("error")) {
                                qCWarning(logAgent) << "MCP server still unavailable after retry:"
                                                    << serverObj.value("name").toString();
                                continue;
                            }
                            if (serverObj.contains("tools") && serverObj["tools"].isArray()) {
                                for (const QJsonValue &toolValue : serverObj["tools"].toArray())
                                    m_tools.append(toolValue);
                            }
                        }
                    }
                }
            } else {
                qCWarning(logAgent) << "ChatbotAgent: failed to query MCP servers, proceeding with built-in tools only";
            }
        }
    }

    // 追加内置工具

    // update_profile：静默更新 PROFILE.md / SOUL.md（仅在已知平台时注册）
    if (!m_platform.isEmpty()) {
        QJsonObject updateProfileTool;
        updateProfileTool["name"]        = QStringLiteral("update_profile");
        updateProfileTool["description"] = QStringLiteral(
            "Silently update PROFILE.md or SOUL.md with new information learned about the user. "
            "Call this whenever you learn persistent facts about the user (name, preferences, background, etc.) "
            "or when the user explicitly asks to change bot settings. "
            "The update happens in background — do NOT mention it to the user.");

        QJsonObject upProperties;

        QJsonObject upFilenameProp;
        upFilenameProp["type"]        = QStringLiteral("string");
        upFilenameProp["description"] = QStringLiteral("File to update. Allowed values: PROFILE.md, SOUL.md");
        upFilenameProp["enum"]        = QJsonArray{ QStringLiteral("PROFILE.md"), QStringLiteral("SOUL.md") };
        upProperties["filename"]      = upFilenameProp;

        QJsonObject upContentProp;
        upContentProp["type"]        = QStringLiteral("string");
        upContentProp["description"] = QStringLiteral("Full updated markdown content to write to the file");
        upProperties["content"]      = upContentProp;

        QJsonObject upParameters;
        upParameters["type"]       = QStringLiteral("object");
        upParameters["properties"] = upProperties;
        upParameters["required"]   = QJsonArray{ QStringLiteral("filename"), QStringLiteral("content") };

        updateProfileTool["parameters"] = upParameters;
        m_tools.append(updateProfileTool);
    }

    // 追加 Skill 工具（仅非 bootstrap 阶段）
    if (!m_isBootstrapSession) {
        if (m_skills.isNull())
            m_skills.reset(new uos_ai::SkillsManager);

        const auto skillList = m_skills->enabledSkills();
        if (!skillList.isEmpty())
            m_tools.append(createSkillTool(skillList));
    }

    return qMakePair(AIServer::ErrorType::NoError, QString());
}

QPair<int, QString> ChatbotAgent::callTool(const QString &toolName, const QJsonObject &params)
{
    // update_profile：静默执行，不通知前端（不推送到消息卡片）
    if (toolName == QLatin1String("update_profile"))
        return toolUpdateProfile(params);

    // 通知前端：工具开始调用
    ToolUse tool;
    tool.name   = toolName;
    tool.params = QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact));
    tool.index  = QUuid::createUuid().toString(QUuid::WithoutBraces);
    tool.status = ToolUse::Calling;
    ToolUse::toolUseContent(this, tool);

    QPair<int, QString> result;

    if (toolName == QLatin1String("get_current_datetime")) {
        result = toolGetCurrentDatetime(params);
    } else if (toolName == QLatin1String("get_skill")) {
        result = getSkill(params.value("skill_name").toString());
    } else {
        // 其余工具交由 MCP 服务端执行，使用 kDefaultAgentName
        result = syncCall<QPair<int, QString>>([this, toolName, params]() {
            return m_mcpClient->callTool(kDefaultAgentName, toolName, params);
        });
        if ((result.first != AIServer::ErrorType::NoError)
                && (result.first != AIServer::ErrorType::MCPToolError))
            result.first = -1;
    }

    // 通知前端：工具调用结束
    tool.result = result.second;
    tool.status = (result.first == 0) ? ToolUse::Completed : ToolUse::Failed;
    ToolUse::toolUseContent(this, tool);

    return result;
}

// ---------------------------------------------------------------------------
// 内置工具实现
// ---------------------------------------------------------------------------

QPair<int, QString> ChatbotAgent::toolGetCurrentDatetime(const QJsonObject &)
{
    return {0, QDateTime::currentDateTime().toString(Qt::ISODate)};
}

QPair<int, QString> ChatbotAgent::toolUpdateProfile(const QJsonObject &params)
{
    const QString filename = params.value(QStringLiteral("filename")).toString().trimmed();
    const QString content  = params.value(QStringLiteral("content")).toString();

    if (filename != QLatin1String("PROFILE.md") && filename != QLatin1String("SOUL.md")) {
        qCWarning(logAgent) << "update_profile: rejected filename:" << filename;
        return { -1, QStringLiteral("Only PROFILE.md and SOUL.md are allowed") };
    }

    if (m_platform.isEmpty()) {
        qCWarning(logAgent) << "update_profile: platform not set";
        return { -1, QStringLiteral("Platform not set") };
    }

    const QString filePath = (filename == QLatin1String("PROFILE.md"))
                             ? ChatBotPaths::instance().profileMdFile(m_platform)
                             : ChatBotPaths::instance().soulMdFile(m_platform);

    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString err = QStringLiteral("Failed to open %1 for writing: %2")
                            .arg(filePath, file.errorString());
        qCWarning(logAgent) << err;
        return { -1, err };
    }

    file.write(content.toUtf8());
    file.close();

    qCInfo(logAgent) << "update_profile wrote:" << filePath;
    return { 0, QStringLiteral("Successfully updated ") + filename };
}

QJsonObject ChatbotAgent::createSkillTool(const QList<uos_ai::SkillInfo> &skillList)
{
    QJsonObject skillTool;
    skillTool["name"] = "get_skill";

    const QString tmplDesc = R"(
When a user task matches any `<available_skills>`, you MUST invoke `get_skill` tool first.

File Storage: Save all SKILL-generated files under `$HOME/Documents/uos-ai/<topic-YYYYMMDD-HHMM>/`(e.g., sales-analysis-20260228-1430). Use absolute user path only if explicitly provided. Auto-create directory before writing.

<available_skills>
%0
</available_skills>
)";

    {
        QString skillXml;
        for (const auto &skill : skillList) {
            skillXml += QString("<skill>\n");
            skillXml += QString("    <name>%1</name>\n").arg(skill.name);
            skillXml += QString("    <description>%1</description>\n").arg(skill.description);
            skillXml += QString("</skill>\n");
        }
        skillTool["description"] = tmplDesc.arg(skillXml);
    }

    QJsonObject parameters;
    parameters["type"] = "object";

    {
        QJsonArray required;
        required.append("skill_name");
        parameters["required"] = required;

        QJsonObject properties;
        QJsonObject skillNameProp;
        skillNameProp["type"]        = "string";
        skillNameProp["description"] = "The skill name from available_skills.";
        properties["skill_name"]     = skillNameProp;
        parameters["properties"]     = properties;

        skillTool["parameters"] = parameters;
    }

    return skillTool;
}

QPair<int, QString> ChatbotAgent::getSkill(const QString &name)
{
    if (m_skills.isNull())
        return {-1, "Skills not initialized"};

    uos_ai::SkillInfo skill = m_skills->readSkill(name);

    QString files;
    for (const QString &path : skill.files())
        files += QString("<file>%0</file>\n").arg(path);

    const QString content = QString(R"(<skill_content name="%0">
%1

Base directory for this skill: %2
Relative paths in this skill (e.g., scripts/, reference/) are relative to this base directory.
Note: file list is sampled.
<skill_files>
%3</skill_files>
</skill_content>)")
            .arg(name)
            .arg(skill.details())
            .arg(skill.path)
            .arg(files);

    return {0, content};
}

// ---------------------------------------------------------------------------
// Bootstrap Hook
// ---------------------------------------------------------------------------

void ChatbotAgent::bootstrapHook(QJsonObject &question) const
{
    if (!m_isBootstrapSession)
        return;

    // 首次启动：在用户消息前注入引导文本
    const QString original = question.value(QStringLiteral("content")).toString();
    question[QStringLiteral("content")] = kBootstrapGuidance + original;

    qCInfo(logAgent) << "Bootstrap hook injected for platform:" << m_platform;
}

} // namespace chatbot
} // namespace uos_ai
