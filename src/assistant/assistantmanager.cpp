#include "assistantmanager.h"
#include "abstractassistant.h"
#include "uosaiaassistant.h"
#include "aiwriter.h"
#include "aitranslation.h"
#include "personalknowledgeassistant.h"
#include "uosclaw.h"
#include "chatassistant.h"
#include "modelvendor.h"
#include "externalllm/externalpluginmanager.h"
#include "chatbot/chatbotassistant.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAssistant)

using namespace uos_ai;

AssistantManager* AssistantManager::instance()
{
    static AssistantManager am;
    return &am;
}

AssistantManager::AssistantManager(QObject *parent) : QObject(parent)
{
    initDefaultAssistants();
}

AssistantManager::~AssistantManager()
{

}

void AssistantManager::initDefaultAssistants()
{
    {
        AssistantInfo uosAiInfo;
        uosAiInfo.id = STR_KEY_UOS_AI_GENERIC;
        uosAiInfo.name = tr("UOS AI");
        uosAiInfo.description = tr("Hello, I'm UOS AI.");
        
        // Add icons: imageType as key, iconName as value
        uosAiInfo.icons["line"] = "uos-ai";
        
        uosAiInfo.path = "icons/";
        assistantsInfo.insert(uosAiInfo.id, uosAiInfo);
    }

    {
        AssistantInfo aiWritingInfo;
        aiWritingInfo.id = STR_KEY_UOS_AI_WRITING;
        aiWritingInfo.name = tr("AI Writing");
        aiWritingInfo.description = tr("Infinite inspiration, worry-free writing");
        
        // Add icons: imageType as key, iconName as value
        aiWritingInfo.icons["line"] = "ai-writing";
        aiWritingInfo.icons["color"] = "ai-writing-color";
        
        aiWritingInfo.path = "icons/";
        aiWritingInfo.placeHolder = tr("Please enter the topic and requirements of the document, and UOS AI will help you complete the creation.");
        assistantsInfo.insert(aiWritingInfo.id, aiWritingInfo);
    }

    {
        AssistantInfo aiTranslationInfo;
        aiTranslationInfo.id = STR_KEY_UOS_AI_TRANSLATION;
        aiTranslationInfo.name = tr("AI Translation");
        aiTranslationInfo.description = tr("Your Translation Assistant, Mastering Multiple Languages.");
        
        // Add icons: imageType as key, iconName as value
        aiTranslationInfo.icons["line"] = "ai-translation";
        aiTranslationInfo.icons["color"] = "ai-translation-color";
        
        aiTranslationInfo.path = "icons/";
        aiTranslationInfo.placeHolder = tr("Please enter the content to be translated and specify the target language; the default target language is Chinese.");
        assistantsInfo.insert(aiTranslationInfo.id, aiTranslationInfo);

    }

#ifdef ENABLE_LOCAL_MODEL
    {
        AssistantInfo knowledgeBaseInfo;
        knowledgeBaseInfo.id = STR_KEY_UOS_AI_KNOWLEDGE_BASE;
        knowledgeBaseInfo.name = tr("AI Knowledge Base");
        knowledgeBaseInfo.description = tr("Answers questions based on your personal knowledge base.");
        
        // Add icons: imageType as key, iconName as value
        knowledgeBaseInfo.icons["line"] = "personal-knowledge-assistant";
        knowledgeBaseInfo.icons["color"] = "personal-knowledge-assistant-color";
        
        knowledgeBaseInfo.path = "icons/";
        knowledgeBaseInfo.placeHolder = tr("Ask questions based on the knowledge base.");
        assistantsInfo.insert(knowledgeBaseInfo.id, knowledgeBaseInfo);
    }
#endif

#ifdef ENABLE_MCP
    {
        AssistantInfo clawInfo;
        clawInfo.id = STR_KEY_UOS_AI_CLAW;
        clawInfo.name = tr("MCP&Skills");
        clawInfo.description = tr("Built-in common MCP & Skills, supports custom tool import, freely choose tools for conversation.");
        
        // Add icons: imageType as key, iconName as value
        clawInfo.icons["line"] = "uos-claw";
        clawInfo.icons["color"] = "uos-claw-color";
        
        clawInfo.path = "icons/";
        clawInfo.placeHolder = tr("Please enter MCP&Skills commands.");
        assistantsInfo.insert(clawInfo.id, clawInfo);
    }
#endif

}

QList<AssistantInfo> AssistantManager::getAssistantList() const
{
    QList<AssistantInfo> result = assistantsInfo.values();
    // 按以下顺序排序
    QList<QString> order = {STR_KEY_UOS_AI_GENERIC, STR_KEY_UOS_AI_WRITING, STR_KEY_UOS_AI_KNOWLEDGE_BASE, STR_KEY_UOS_AI_CLAW, STR_KEY_UOS_AI_TRANSLATION};
    std::stable_sort(result.begin(), result.end(), [order](const AssistantInfo &a, const AssistantInfo &b) {
        int aIndex = order.indexOf(a.id);
        int bIndex = order.indexOf(b.id);
        if (aIndex < 0 ) {
            return false;
        }

        if (bIndex < 0) {
            return true;
        }
        
        return aIndex < bIndex;
    });
    
    auto externalAssistants = ExternalPluginManager::instance()->getExternalAssistants();
    for (const auto &assistant : externalAssistants) {
        if (!assistantsInfo.contains(assistant.id)) {
            result.append(assistant);
        } else {
            qCWarning(logAssistant) << "AssistantManager: Assistant with id " << assistant.id << " already exists.";
        }
    }
    
    return result;
}

QList<ModelAccountPtr> AssistantManager::availableModels(const QString &id) const
{
    QVariantHash condition;
    condition.insert(STR_KEY_ARCH, ModelArch::MaLanguage);
    if (id == STR_KEY_UOS_AI_GENERIC) {
        QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText))};
        condition.insert(STR_KEY_ABILITY, ablity);
    } else if (id == STR_KEY_UOS_AI_WRITING) {
        QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall))};
        condition.insert(STR_KEY_ABILITY, ablity);
    } else if (id == STR_KEY_UOS_AI_CLAW) {
        QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall))};
        condition.insert(STR_KEY_ABILITY, ablity);
    } else if (id == STR_KEY_UOS_AI_TRANSLATION) {
        QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText))};
        condition.insert(STR_KEY_ABILITY, ablity);
    } else if (id == STR_KEY_UOS_AI_KNOWLEDGE_BASE) {
        QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText))};
        condition.insert(STR_KEY_ABILITY, ablity);
    } else if (id == STR_KEY_UOS_AI_CHAT) {
        QVariantList ablity{QVariant(ModelAbilities(ModelAbility::MaText))};
        condition.insert(STR_KEY_ABILITY, ablity);
    } else if (ExternalPluginManager::instance()->hasAssistant(id)) {
        return ExternalPluginManager::instance()->getModelsByAssistant(id);
    }

    QList<ModelAccountPtr> models = ModelVendor::instance()->queryModels(condition);    
    return models;
}

AssistantPtr AssistantManager::createAssistant(const QString &id) const
{
    AssistantPtr obj;
    if (id == STR_KEY_UOS_AI_GENERIC) {
        obj = AssistantPtr(new UOSAIAssistant());
    } else if (id == STR_KEY_UOS_AI_WRITING) {
        obj = AssistantPtr(new AIWriter());
    } else if (id == STR_KEY_UOS_AI_TRANSLATION) {
        obj = AssistantPtr(new AITranslation());
    } else if (id == STR_KEY_UOS_AI_KNOWLEDGE_BASE) {
        obj = AssistantPtr(new PersonalKnowledgeAssistant());
    } else if (id == STR_KEY_UOS_AI_CLAW) {
        obj = AssistantPtr(new UOSClaw());
    } else if (id == STR_KEY_UOS_AI_CHAT) {
        obj = AssistantPtr(new ChatAssistant());
    } else if (id == chatbot::STR_KEY_UOS_AI_CHATBOT) {
        obj = AssistantPtr(new chatbot::ChatBotAssistant());
    } else if (ExternalPluginManager::instance()->hasAssistant(id)) {
        obj = AssistantPtr(new ChatAssistant());
    }

    return obj;
}
