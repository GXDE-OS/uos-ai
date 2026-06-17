#include "defaultagentwithskills.h"
#include "bashwithsandbox.h"
#include "global_key_define.h"

#include "utils/report/eventlogutil.h"
#include "utils/report/mcpchatpoint.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTextStream>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

DefaultAgentWithSkills::DefaultAgentWithSkills(QObject *parent) : DefaultAgent(parent)
{

}

QPair<int, QString> DefaultAgentWithSkills::fetchTools(const QStringList &servers)
{
    QPair<int, QString> ret = DefaultAgent::fetchTools(servers);
    if (servers.contains("uos-mcp", Qt::CaseInsensitive)) {
        m_tools.append(createBashTool());
    }

    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);

    auto skillList = m_skills->enabledSkills();
    if (!skillList.isEmpty())
        m_tools.append(createSkillTool(skillList));
    m_tools.append(createInstallSkillTool());

    return ret;
}

QVariantHash DefaultAgentWithSkills::processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params)
{
    if (params.value("enable_sandbox", true).toBool())
        qCInfo(logAgent) << "user enable sandbox";
    else
        qCWarning(logAgent) << "user disable sandbox, will execute bash command directly!!!";
    m_alwaysApprove = params.value(STR_KEY_ALWAYS_APPROVE, false).toBool();

    if (m_alwaysApprove) {
        qCWarning(logAgent) << "user always approve bash command";
    }

    m_reqQarams = params;
    return DefaultAgent::processRequest(question, history, params);
}

void DefaultAgentWithSkills::cancel()
{
    LlmAgent::cancel();
    emit agentCancled();
}

void DefaultAgentWithSkills::invokeAction(const QJsonObject &action)
{
    emit actionCallback(action);
}

QPair<int, QString> DefaultAgentWithSkills::callTool(const QString &toolName, const QJsonObject &params)
{
    ReportIns()->writeEvent(report::MCPChatPoint(toolName).assemblingData());
    if (toolName == QString("install_skill")) {
        QString strParams = QString::fromUtf8(QJsonDocument(params).toJson());
        QString id = GlobalUtil::generateMsId();
        int status = NormalStatus::NsRunning;
        emit messageReceived({RenderMessage::createTool(id, toolName, strParams, status)});

        QString source = params.value("source").toString();
        auto ret = installSkill(source);

        status = ret.first < 0 ? NormalStatus::NsFailed : NormalStatus::NsCompleted;
        emit messageReceived({RenderMessage::createTool(id, toolName, strParams, status, ret.second)});
        return ret;
    }

    if (toolName == QString("get_skill") || toolName == QString("run_bash")) {
            auto tmpParams = params;
            tmpParams.remove("explain");
            QPair<int, QString> ret;
            QString explain = params.value("explain").toString();
            QString strParams = QString::fromUtf8(QJsonDocument(tmpParams).toJson());
            QString id = GlobalUtil::generateMsId();
            int status = NormalStatus::NsRunning;
            emit messageReceived({RenderMessage::createTool(id, toolName, strParams, status)});

        if (toolName == QString("get_skill")) {
            QString name = params.value("skill_name").toString();
            ret = getSkill(name);
        } else {
            BashWithSandbox bash(m_alwaysApprove);
            connect(&bash, &BashWithSandbox::sendRequsets, this, &DefaultAgentWithSkills::messageReceived, Qt::DirectConnection);
            connect(this, &DefaultAgentWithSkills::actionCallback, &bash, &BashWithSandbox::actionCallback, Qt::DirectConnection);
            connect(this, &DefaultAgentWithSkills::agentCancled, &bash, &BashWithSandbox::abort, Qt::DirectConnection);
            if (canceled) {
                ret = {GErrorType::MCPToolError, "canceled"};
            } else {
                QString cmd = params.value("command").toString();
                QString cwd = params.value("workdir").toString();
                if (cmd.isEmpty())
                    ret = {GErrorType::MCPToolError, "no command"};
                else
                    ret = bash.execute(cmd, cwd, explain, m_reqQarams.value("enable_sandbox", true).toBool());
                m_alwaysApprove = bash.skipApproval();
            }
        }

        status = ret.first < 0 ? NormalStatus::NsFailed : NormalStatus::NsCompleted;
        emit messageReceived({RenderMessage::createTool(id, toolName, strParams, status, ret.second)});
        return ret;
    }

    return DefaultAgent::callTool(toolName, params);
}

ModelTool DefaultAgentWithSkills::createSkillTool(const QList<SkillInfo> &skillList)
{
    ModelTool skillTool;
    skillTool.name = "get_skill";

    QString tmplDesc = R"(
When a user task matches any `<available_skills>`, you MUST invoke `get_skill` tool first.

File Storage: Save all SKILL-generated files under `$HOME/Documents/uos-ai/<topic-YYYYMMDD-HHMM>/`(e.g., sales-analysis-20260228-1430). Use absolute user path only if explicitly provided. Auto-create directory before writing.

%2

<available_skills>
%1
</available_skills>
)";

    {
        QString skillXml;
        for (auto it = skillList.begin(); it != skillList.end(); ++it) {
            skillXml += QString("<skill>\n");
            skillXml += QString("    <name>%1</name>\n")
                            .arg(it->name);
            skillXml += QString("    <description>%1</description>\n")
                            .arg(it->description);
            skillXml += QString("</skill>\n");
        }
        skillTool.description = tmplDesc.arg(skillXml, skillPathsContext());
    }

    skillTool.required.append("skill_name");

    {
        ModelToolProperty skillNameProp;
        skillNameProp.name = "skill_name";
        skillNameProp.type = "string";
        skillNameProp.description = "The skill name from available_skills.";
        skillTool.properties.append(skillNameProp);
    }

    return skillTool;
}

ModelTool DefaultAgentWithSkills::createInstallSkillTool()
{
    ModelTool tool;
    tool.name = "install_skill";
    tool.description = QStringLiteral("Install a UOS AI skill from a local skill directory, SKILL.md file, .skill/.zip/.tar.* archive, or a direct http(s) download URL. Use this when the user asks to install, import, or add a skill. Installed skills are stored under the UOS AI user skill install path as <skillName> subdirectories.\n\n") + skillPathsContext();
    tool.required.append("source");

    ModelToolProperty sourceProp;
    sourceProp.name = "source";
    sourceProp.type = "string";
    sourceProp.description = QStringLiteral("Local path or direct http(s) URL pointing to a SKILL.md file or supported skill archive.");
    tool.properties.append(sourceProp);

    return tool;
}

QString DefaultAgentWithSkills::skillPathsContext() const
{
    QString context;
    QTextStream stream(&context);
    const QStringList searchPaths = SkillsManager::allSkillSearchPaths();

    stream << "<UOS_AI_SKILL_PATHS>\n";
    stream << "User skill install path: " << SkillsManager::userSkillInstallPath() << "\n";
    stream << "Skill config file: " << SkillsManager::skillsConfigFilePath() << "\n";
    stream << "Builtin skill path: " << SkillsManager::builtinSkillPath() << "\n";
    stream << "Search paths in priority order:\n";
    for (int i = 0; i < searchPaths.size(); ++i)
        stream << i + 1 << ". " << searchPaths.at(i) << "\n";
    stream << "Use the user skill install path for newly created or imported UOS AI skills. Do not use $XDG_SKILLS_HOME as a UOS AI skill storage path unless the user explicitly asks.\n";
    stream << "</UOS_AI_SKILL_PATHS>";
    return context;
}

QPair<int, QString> DefaultAgentWithSkills::installSkill(const QString &source)
{
    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);

    QString errorMsg;
    QString skillName;
    const bool success = m_skills->addSkillSource(source, &errorMsg, &skillName);
    if (success)
        m_skills->reloadSkills();

    QJsonObject result;
    result[QStringLiteral("success")] = success;
    if (success) {
        result[QStringLiteral("skill_name")] = skillName;
        result[QStringLiteral("message")] = tr("Skill '%1' installed successfully.").arg(skillName);
    } else {
        result[QStringLiteral("error")] = errorMsg.isEmpty() ? tr("Failed to install the skill.") : errorMsg;
    }

    return { GErrorType::NoError, QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)) };
}

ModelTool DefaultAgentWithSkills::createBashTool()
{
    ModelTool bashTool;
    bashTool.name = "run_bash";
    bashTool.description = "Executes a given bash command. Ensure all commands are executed non-interactively, without requiring any user input or launching interactive programs. This is the system's default bash tool. If multiple bash tools are available, you MUST prefer this tool to execute bash commands.";

    bashTool.required.append("command");

    {
        ModelToolProperty commandProp;
        commandProp.name = "command";
        commandProp.type = "string";
        commandProp.description = "The command to execute";
        bashTool.properties.append(commandProp);
    }

    {
        ModelToolProperty workdirProp;
        workdirProp.name = "workdir";
        workdirProp.type = "string";
        workdirProp.description = "The working directory to run the command in.";
        bashTool.properties.append(workdirProp);
    }

    {
        ModelToolProperty explainProp;
        explainProp.name = "explain";
        explainProp.type = "string";
        explainProp.description = "A clear, concise active-voice question that asks the user whether to perform the command's action. Follow the style of: 'Do you want me to delete the file?'";
        bashTool.properties.append(explainProp);
    }

    return bashTool;
}

QPair<int, QString> DefaultAgentWithSkills::getSkill(const QString &name)
{
    SkillInfo skill = m_skills->readSkill(name);

    QString files;
    for (const QString &path : skill.files()) {
        files += QString("<file>%0</file>\n").arg(path);
    }

    QString content = QString(R"(<skill_content name="%0">
%1

Base directory for this skill: %2
Relative paths in this skill (e.g., scripts/, reference/) are relative to this base directory.
Note: file list is sampled.
<skill_files>
%3</skill_files>
</skill_content>)").arg(name)
            .arg(skill.details())
            .arg(skill.path)
            .arg(files);

    return {0, content};
}
