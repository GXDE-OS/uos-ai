#include "assistantchannel.h"
#include "assistant/assistantmanager.h"
#include "assistant/aiwriter.h"
#include "assistant/aitranslation.h"
#include "assistant/uosclaw.h"
#include "global_key_define.h"
#include "database/appdatabase.h"
#include "database/usedmodeltable.h"
#include "model/modelvendor.h"
#include "services/accountservice/freeaccountservice.h"
#include "externalllm/externalpluginmanager.h"
#include "app/application.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logAIGUI)
using namespace uos_ai;

namespace {
// 与前端默认常驻助手数量保持一致，兼容首次启动或旧版本没有配置的情况。
constexpr int kDefaultAssistantVisibleCount = 4;
}

AssistantChannel::AssistantChannel(QObject *parent)
    : QObject(parent)
{
    connect(FreeAccountService::instance(), &FreeAccountService::claimUsageComplete, this, &AssistantChannel::claimUsageComplete);
    connect(ExternalPluginManager::instance(), &ExternalPluginManager::assistantChanged, this , &AssistantChannel::assistantChanged, Qt::QueuedConnection);
    connect(ModelVendor::instance(), &ModelVendor::modelChanged, this, &AssistantChannel::modelListChanged, Qt::QueuedConnection);
}

AssistantChannel::~AssistantChannel()
{
}

QJsonArray AssistantChannel::getAssistantList()
{
    auto list = AssistantMgr->getAssistantList();

    QJsonArray root;
    for (const AssistantInfo &info : list) {
        root.append(info.toJson());
    }

    return root;
}

void AssistantChannel::setAssistantOrder(const QJsonArray &assistantList)
{
    QJsonDocument doc(assistantList);
    AppDatabase::instance()->saveConfig(CONFIG_ASSISTANT_ORDER, QString::fromUtf8(doc.toJson()));
}

QJsonArray AssistantChannel::getAssistantOrder()
{
    QJsonArray ret;
    QString json = AppDatabase::instance()->getConfig(CONFIG_ASSISTANT_ORDER);
    if (json.isEmpty())
        return ret;

    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    return doc.array();
}

void AssistantChannel::setAssistantVisibleCount(int visibleCount)
{
    // 前端已做范围归一化，这里只负责落库，保持通道层逻辑轻量。
    AppDatabase::instance()->saveConfigInt(CONFIG_ASSISTANT_VISIBLE_COUNT, visibleCount);
}

int AssistantChannel::getAssistantVisibleCount()
{
    QString value = AppDatabase::instance()->getConfig(CONFIG_ASSISTANT_VISIBLE_COUNT);
    if (value.isEmpty())
        // 没有历史配置时返回默认常驻数量，避免前端出现空值分支。
        return kDefaultAssistantVisibleCount;

    return value.toInt();
}

void AssistantChannel::claimUsageRequest(const QString &modelId)
{
    FreeAccountService::instance()->claimUsageRequest(modelId);
}

QJsonArray AssistantChannel::getModelList(const QString &assistantId)
{
    auto list = AssistantMgr->availableModels(assistantId);
    QJsonArray root;
    for (const ModelAccountPtr &info : list) {
        QJsonObject obj;
        obj.insert(STR_KEY_ID, info->id);
        obj.insert(STR_KEY_NAME, info->model.name);
        obj.insert(STR_KEY_NETWORK, info->network);
        obj.insert(STR_KEY_PROVIDER, info->account.provider);
        obj.insert(STR_KEY_ICON, info->model.icon);
        obj.insert(STR_KEY_ABILITY, static_cast<int>(info->model.ability));
        root.append(obj);
    }

    return root;
}

QString AssistantChannel::getCurrentModel(const QString &assistantId)
{
    auto *db = AppDatabase::instance();
    UsedModelTable usedModel = UsedModelTable::getByAssistant(db, assistantId);

    if (!usedModel.assistant().isEmpty()) {
        return usedModel.model();
    }
    
    // 找不到当前模型，返回默认模型
    auto models = AssistantMgr->availableModels(assistantId);
    // 过滤出uos_ai模型
    for (const ModelAccountPtr &model : models) {
        if (ModelVendor::isUosProvider(model)) {
            return model->id;
        }
    }
    
    if (!models.isEmpty()) {
        return models.first()->id;
    }
    
    // 模型列表为空，返回空字符串
    return "";
}

bool AssistantChannel::setCurrentModel(const QString &modelId, const QString &assistantId)
{
    auto *db = AppDatabase::instance();
    UsedModelTable usedModel = UsedModelTable::getByAssistant(db, assistantId);
    
    if (!usedModel.assistant().isEmpty()) {
        // Update existing record
        usedModel.setModel(modelId);
        return usedModel.update(db);
    } else {
        // Create new record
        UsedModelTable newRecord = UsedModelTable::create(assistantId, modelId, "");
        return newRecord.save(db);
    }
}

QString AssistantChannel::getRecentWritingDocs()
{
    return AIWriter::getRecentDocs();
}

QString AssistantChannel::getWritingTemplates()
{
    return AIWriter::getWritingTemplates();
}

QString AssistantChannel::getTranslationFAQ()
{
    return AITranslation::getTranslationFAQ();
}

QString AssistantChannel::getClawFAQ()
{
    return UOSClaw::faq();
}

void AssistantChannel::requestAddModel()
{
    aiApp->showConfig(MgmtWindow::Page::AddModel);
}
