#ifndef ASSISTANTMANAGER_H
#define ASSISTANTMANAGER_H

#include "assistantinfo.h"
#include "abstractassistant.h"
#include "model/modelinfo.h"

#include <QObject>
#include <QMap>

namespace uos_ai {

// Assistant ID constants
inline constexpr char STR_KEY_UOS_AI_GENERIC[] = "uos-ai-generic";
inline constexpr char STR_KEY_UOS_AI_WRITING[] = "uos-ai-writing";
inline constexpr char STR_KEY_UOS_AI_TRANSLATION[] = "uos-ai-translation";
inline constexpr char STR_KEY_UOS_AI_KNOWLEDGE_BASE[] = "uos-ai-knowledge-base";
inline constexpr char STR_KEY_UOS_AI_CLAW[] = "uos-ai-claw";
inline constexpr char STR_KEY_UOS_AI_CHAT[] = "uos-ai-chat";

class AssistantManager : public QObject {
    Q_OBJECT
public:
    static AssistantManager* instance();
    QList<AssistantInfo> getAssistantList() const;
    QList<ModelAccountPtr> availableModels(const QString& id) const;
    AssistantPtr createAssistant(const QString& id) const;

private:
    explicit AssistantManager(QObject *parent = nullptr);
    ~AssistantManager();
    void initDefaultAssistants();

private:
    QMap<QString, AssistantInfo> assistantsInfo;
};

} // namespace uos_ai

#define AssistantMgr uos_ai::AssistantManager::instance()

#endif // ASSISTANTMANAGER_H
