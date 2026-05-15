#include "modelvendor.h"
#include "builtinprovider.h"
#include "global_key_define.h"
#include "database/appdatabase.h"
#include "externalllm/externalpluginmanager.h"

#include "uosfreeaccounts.h"
#include "network/httpcodetranslation.h"

#include "openai/openaicompatibleprovider.h"
#include "uosfree/uosfreeprovider.h"
#include "deepseek/deepseekprovider.h"
#include "minimax/minimaxprovider.h"
#include "moonshot/moonshotprovider.h"
#include "volcengine/volcengineprovider.h"
#include "bigmodel/bigmodelprovider.h"
#include "bailian/bailianprovider.h"
#include "modelhub/modelhubprovider.h"
#include "coze/cozeprovider.h"
#include "llmplugin/llmpluginprovider.h"

#include <QThread>
#include <QApplication>
#include <QtConcurrent>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

ModelVendor::ModelVendor(QObject *parent) : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    BuiltinProvider::instance();
#ifdef ENABLE_LOCAL_MODEL
    m_modelhub.reset(new ModelHubProvider);
#endif
}

ModelVendor *ModelVendor::instance()
{
    static ModelVendor ins;
    return &ins;
}

void ModelVendor::refresh()
{
    m_accounts.clear();
    auto db = AppDatabase::instance();

    auto providers = db->queryAllProviders();
    for (auto it = providers.begin(); it != providers.end();) {
        if (!BuiltinProvider::instance()->isProviderSupported(it->provider)) {
            qCWarning(logModel) << "Provider not supported:" << it->id << it->name << it->provider;
            it = providers.erase(it);
            continue;
        }
        it++;
    }

    auto models = db->queryAllModels();

    for (ModelAccountPtr model : models) {
        if (providers.contains(model->account.id)) {
            auto provider = providers.value(model->account.id);
            model->account = provider;
            model->network = networkType(model);
            if (!model->model.id.isEmpty()) {
                if (BuiltinProvider::instance()->isModelSupported(model->model.id)) {
                    model->model = BuiltinProvider::instance()->getModelInfo(model->model.id);
                } else {
                    qCWarning(logModel) << "Model not supported:" << model->id << model->model.name << "provider:" << model->account.id
                                        << "model id" << model->model.id << model->model.modelId;
                    continue;
                }
            }
        } else {
            qCWarning(logModel) << "No provider found for model:" << model->id << model->model.name << "need provider:" << model->account.id;
            continue;
        }

        m_accounts.insert(model->id, model);
    }

    ExternalPluginManager::instance()->refresh();

    emit modelChanged();
}

void ModelVendor::refreshLocal() const
{
#ifdef ENABLE_LOCAL_MODEL
    m_local.clear();

    for (auto m : m_modelhub->modelList())
        m_local.insert(m->id, m);
#endif
}

QList<ModelAccountPtr> ModelVendor::queryModels(const QVariantHash &condition) const
{
    // 本地模型的安装与卸载难以监听，因此在每次查询时刷新本地模型
    refreshLocal();

    QList<ModelAccountPtr> ret = m_accounts.values();
    ret.append(m_local.values());

    if (condition.isEmpty()) {
        return ret;
    }

    if (condition.contains(STR_KEY_ARCH)) {
        auto arch = condition.value(STR_KEY_ARCH).toInt();
        for (auto it = ret.begin(); it != ret.end();) {
            if ((*it)->model.arch != ModelArch(arch)) {
                it = ret.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (condition.contains(STR_KEY_ABILITY)) {
        QList<ModelAbilities> abilities;
        for (const QVariant &v : condition.value(STR_KEY_ABILITY).toList()) {
            abilities.append(ModelAbilities(v.toInt()));
        }

        for (auto it = ret.begin(); it != ret.end();) {
            bool found = false;
            for (ModelAbilities ability : abilities) {
                if (((*it)->model.ability & ability) == ability){
                    found = true;
                    break;
                }
            }

            if (!found) {
                it = ret.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    return ret;
}

ModelAccountPtr ModelVendor::getModel(const QString &id) const
{
    if (m_accounts.contains(id))
        return m_accounts.value(id);

    if (m_local.contains(id))
        return  m_local.value(id);

    return ExternalPluginManager::instance()->getModel(id);
}

QSharedPointer<AbstractModel> ModelVendor::createModel(const ModelAccountPtr &acc) const
{
    if (!acc) {
        qCWarning(logModel) << "Invalid model account provided";
        return nullptr;
    }

    // 根据provider类型创建对应的模型实例
    QSharedPointer<ModelProvider> providerPtr = provider(acc->account.provider);
    if (!providerPtr) {
        qCWarning(logModel) << "No provider found for:" << acc->account.provider;
        return nullptr;
    }

    AbstractModel *model = providerPtr->createModel(acc);
    if (!model) {
        qCWarning(logModel) << "Failed to create model for account:" << acc->id;
        return nullptr;
    }

    return QSharedPointer<AbstractModel>(model);
}

QSharedPointer<ModelProvider> ModelVendor::provider(const QString &id) const
{
    QSharedPointer<ModelProvider> ret;
    if (id.compare(STR_KEY_UOS_AI, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new UosFreeProvider());
    } else if (id.compare(STR_KEY_OPENAI_COMPATIBLE, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new OpenaiCompatibleProvider());
    } else if (id.compare(STR_KEY_PRIVATE_NET, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new OpenaiCompatibleProvider());
    } else if (id.compare(STR_KEY_DEEPSEEK, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new DeepSeekProvider());
    }  else if (id.compare(STR_KEY_MINIMAX, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new MiniMaxProvider());
    } else if (id.compare(STR_KEY_MOONSHOT, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new MoonshotProvider());
    } else if (id.compare(STR_KEY_VOLCENGINE, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new VolcengineProvider());
    } else if (id.compare(STR_KEY_BIGMODEL, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new BigModelProvider());
    } else if (id.compare(STR_KEY_BAILIAN, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new BailianProvider());
    } else if (id.compare(STR_KEY_MODELHUB, Qt::CaseInsensitive) == 0) {
#ifdef ENABLE_LOCAL_MODEL
        ret = m_modelhub;
#endif
    } else if (id.compare(STR_KEY_COZE_AGENT, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new CozeProvider());
    } else if (id.compare(STR_KEY_LLM_PLUGIN, Qt::CaseInsensitive) == 0) {
        ret = QSharedPointer<ModelProvider>(new LLMPluginProvider());
    } else {
        qCWarning(logModel) << "Unknown provider id:" << id;
    }

    return ret;
}

void ModelVendor::removeProvider(const QString &id)
{
    bool change = false;
    for (auto it = m_accounts.begin(); it != m_accounts.end();) {
        if (it->data()->account.id == id) {
            change = true;
            it = m_accounts.erase(it);
        } else {
            ++it;
        }
    }

    if (change)
        emit modelChanged();
}

void ModelVendor::removeModel(const QString &id)
{
    if (m_accounts.contains(id)) {
        m_accounts.remove(id);
        emit modelChanged();
    }

    return;
}

QString ModelVendor::networkType(const ModelAccountPtr &acc)
{
    QString providerType = acc->account.provider;
    if (providerType.compare(STR_KEY_PRIVATE_NET, Qt::CaseInsensitive) == 0)
        return STR_KEY_PRIVATE;

    return STR_KEY_ONLINE;
}

bool ModelVendor::isValid(const ModelAccountPtr &acc)
{
    if (!acc.data())
        return false;

    if (acc->id.isEmpty())
        return false;

    if (acc->account.id.isEmpty())
        return false;
    
    if (acc->model.id.isEmpty()) {
        if (acc->model.modelId.isEmpty())
            return false;
        else
            return true;
    }

    if (BuiltinProvider::instance()->isModelSupported(acc->model.id))
        return true;
        
    return false;
}

bool ModelVendor::isUosProvider(const ModelAccountPtr &acc)
{
    if (!acc.data())
        return false;

    return isUosProvider(acc->account.provider);
}

bool ModelVendor::isUosProvider(const QString &provider)
{
    return provider.compare(STR_KEY_UOS_AI, Qt::CaseInsensitive) == 0;
}
