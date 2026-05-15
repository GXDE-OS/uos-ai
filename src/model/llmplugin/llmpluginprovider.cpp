#include "llmpluginprovider.h"
#include "llmpluginchatmodel.h"
#include "externalllm/externalpluginmanager.h"

#include <QDebug>

using namespace uos_ai;

LLMPluginProvider::LLMPluginProvider(QObject *parent)
    : ModelProvider(parent)
{
}

LLMPluginProvider::~LLMPluginProvider()
{
}

AbstractModel* LLMPluginProvider::createModel(const ModelAccountPtr &acc)
{
    if (!acc) {
        qWarning() << "Invalid model account provided";
        return nullptr;
    }

    auto plugin = ExternalPluginManager::instance()->getPlugin(acc->id);
    if (!plugin) {
        qWarning() << "Failed to get plugin for model:" << acc->id;
        return nullptr;
    }

    auto *llmModel = plugin->createModel(acc->model.modelId);
    if (!llmModel) {
        qWarning() << "Failed to create LLMModel for model:" << acc->model.modelId;
        return nullptr;
    }

    auto *chatModel = new LLMPluginChatModel();
    chatModel->setAccount(acc);
    chatModel->setLLMModel(llmModel);

    return chatModel;
}
