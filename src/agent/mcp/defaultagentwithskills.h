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
    QPair<int, QString> fetchTools(const QStringList &servers) override;
    QVariantHash processRequest(const ModelMessage &question, const QList<ModelMessage> &history, const QVariantHash &params = {}) override;
    void cancel() override;
signals:
    void actionCallback(const QJsonObject &action);
    void agentCancled();
public Q_SLOTS:
    void invokeAction(const QJsonObject &action) override;
protected:
   QPair<int, QString> callTool(const QString &toolName, const QJsonObject &params) override;
   ModelTool createSkillTool(const QList<SkillInfo> &skillList);
   ModelTool createInstallSkillTool();
   ModelTool createBashTool();
   QString skillPathsContext() const;
   QPair<int, QString> getSkill(const QString &name);
   QPair<int, QString> installSkill(const QString &source);
protected:
    QScopedPointer<SkillsManager> m_skills;
    QVariantHash m_reqQarams;
    bool m_alwaysApprove = false;
};

}



#endif // DEFAULTAGENTWITHSKILLS_H
