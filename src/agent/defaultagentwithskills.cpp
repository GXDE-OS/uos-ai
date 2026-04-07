#include "defaultagentwithskills.h"

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAgent)

using namespace uos_ai;

DefaultAgentWithSkills::DefaultAgentWithSkills(QObject *parent) : DefaultAgent(parent)
{

}

QSharedPointer<LlmAgent> DefaultAgentWithSkills::create()
{
    return QSharedPointer<LlmAgent>(new DefaultAgentWithSkills());
}

QPair<int, QString> DefaultAgentWithSkills::fetchTools(const QStringList &servers)
{
    QPair<int, QString> ret = DefaultAgent::fetchTools(servers);

    if (m_skills.isNull())
        m_skills.reset(new SkillsManager);

    auto skillList = m_skills->enabledSkills();
    if (!skillList.isEmpty())
        m_tools.append(createSkillTool(skillList));

    return ret;
}

QPair<int, QString> DefaultAgentWithSkills::callTool(const QString &toolName, const QJsonObject &params)
{
    if (toolName == QString("get_skill")) {
        QString name = params.value("skill_name").toString();
        return getSkill(name);
    }

    return  MCPChatAgent::callTool(toolName, params);
}

QJsonObject DefaultAgentWithSkills::createSkillTool(const QList<SkillInfo> &skillList)
{
    QJsonObject skillTool;
    skillTool["name"] = "get_skill";
    QString tmplDesc = R"(
When a user task matches any `<available_skills>`, you MUST invoke `get_skill` tool first.

File Storage: Save all SKILL-generated files under `$HOME/Documents/uos-ai/<topic-YYYYMMDD-HHMM>/`(e.g., sales-analysis-20260228-1430). Use absolute user path only if explicitly provided. Auto-create directory before writing.

<available_skills>
%0
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
         skillNameProp["type"] = "string";
         skillNameProp["description"] = "The skil name from available_skills.";
         properties["skill_name"] = skillNameProp;

         parameters["properties"] = properties;

         skillTool["parameters"] = parameters;
    }

    return skillTool;
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
