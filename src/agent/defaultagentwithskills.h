#ifndef DEFAULTAGENTWITHSKILLS_H
#define DEFAULTAGENTWITHSKILLS_H

#include "defaultagent.h"
#include "skillsmanager.h"

namespace uos_ai {

class DefaultAgentWithSkills : public DefaultAgent
{
    Q_OBJECT
public:
    explicit DefaultAgentWithSkills(QObject *parent = nullptr);

    static QSharedPointer<LlmAgent> create();

   QPair<int, QString> fetchTools(const QStringList &servers) override;
protected:
   QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;
   QJsonObject createSkillTool(const QList<SkillInfo> &skillList);
   QPair<int, QString> getSkill(const QString &name);
protected:
    QScopedPointer<SkillsManager> m_skills;
};

}



#endif // DEFAULTAGENTWITHSKILLS_H
