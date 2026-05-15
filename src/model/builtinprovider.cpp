#include "builtinprovider.h"
#include "global_key_define.h"
#include "appdatabase.h"
#include "tas/uosaccountencoder.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>
#include <QApplication>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logModel)

using namespace uos_ai;

// 静态单例实例
BuiltinProvider *BuiltinProvider::instance()
{
    static BuiltinProvider ins;
    return &ins;
}

BuiltinProvider::ProviderInfo BuiltinProvider::queryProvider(const QString &id) const
{
    return m_providers.value(id);
}

ModelInfo BuiltinProvider::getModelInfo(const QString &id) const
{
    return m_models.value(id);
}

void BuiltinProvider::refreshProviders()
{
    m_providers.clear();
    m_models.clear();

    initializeProviders();
    initializeExternalProviders();
}

ProviderAccount BuiltinProvider::xfInline()
{
    ProviderAccount acc;
    acc.auth.insert(STR_KEY_APP_ID, "ae3a30d6");
    acc.auth.insert(STR_KEY_API_KEY, "ca9af472727c12e347202d79f2b0f5a9");
    acc.auth.insert(STR_KEY_API_SECRET, "MGEyMWVhYTRlYTZiNWI5NmE0ZTFkZjc0");
    return acc;
}

BuiltinProvider::BuiltinProvider(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    refreshProviders();
}

bool BuiltinProvider::isModelSupported(const QString &id) const
{
    return m_models.contains(id);
}

bool BuiltinProvider::isProviderSupported(const QString &id) const
{
    return m_providers.contains(id);
}

void BuiltinProvider::initializeProviders()
{
    // 免费账号
    {
        ProviderInfo uosFree;
        uosFree.id = STR_KEY_UOS_AI;
        uosFree.name = tr("UOS AI");
        // 虚拟模型 auto
        {
            ModelInfo model;
            model.id = UOS_FREE_MODEL_AUTO;
            model.name = tr("Intelligent Routing");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);

            uosFree.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // 联网搜索
        {
            ModelInfo model;
            model.id = UOS_FREE_ONLINE_SEARCH;
            model.name = tr("Online Search");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaReasoning);

            uosFree.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // deepseek 3.2
        {
            ModelInfo model;
            model.id = UOS_FREE_DEEPSEEK_V3_2;
            model.name = tr("DeepSeek-V3.2");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "deepseek-v3-2-251201";

            uosFree.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // GLM 4.7
        {
            ModelInfo model;
            model.id = UOS_FREE_GLM_4_7;
            model.name = tr("GLM-4.7");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "glm-4-7-251222";

            uosFree.models.append(model.id);
            m_models.insert(model.id, model);
        }

#if 0
        // Doubao-Seed-1.8
        {
            ModelInfo model;
            model.id = UOS_FREE_DOOUBAO_SEED_1_8;
            model.name = tr("Doubao-Seed-1.8");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaImage | ModelAbility::MaFunctionCalling | ModelAbility::MaReasoning);
            model.modelId = "doubao-seed-1-8-251228";

            uosFree.models.append(model.id);
            m_models.insert(model.id, model);
        }
#endif

        m_providers.insert(uosFree.id, uosFree);
    }
    
     // OpenAI Compatible
    {
        ProviderInfo openaiCompatible;
        openaiCompatible.id = STR_KEY_OPENAI_COMPATIBLE;
        openaiCompatible.name = tr("Custom");
        m_providers.insert(openaiCompatible.id, openaiCompatible);
    }

    // 私有化部署
    {
        ProviderInfo privateDeploy;
        privateDeploy.id = STR_KEY_PRIVATE_NET;
        privateDeploy.name = tr("Private deployment");
        m_providers.insert(privateDeploy.id, privateDeploy);
    }
    
#if 0
    // OpenAI
    {
        ProviderInfo openai;
        openai.id = STR_KEY_OPENAI;
        openai.name = tr("OpenAI");
        // GPT 3.5
        {
            ModelInfo model;
            model.id = OPENAI_GPT_3_5;
            model.name = tr("GPT-3.5 Turbo");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText);
            model.modelId = "gpt-3.5-turbo";

            openai.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // GPT 4
        {
            ModelInfo model;
            model.id = OPENAI_GPT_4;
            model.name = tr("GPT-4");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaFunctionCalling);
            model.modelId = "gpt-4";

            openai.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // GPT 4.1
        {
            ModelInfo model;
            model.id = OPENAI_GPT_4_1;
            model.name = tr("GPT-4.1");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaFunctionCalling);
            model.modelId = "gpt-4.1";

            openai.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // o1
        {
            ModelInfo model;
            model.id = OPENAI_GPT_O_1;
            model.name = tr("o1");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaFunctionCalling | ModelAbility::MaImage);
            model.modelId = "o1";

            openai.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // GPT-5.3
        {
            ModelInfo model;
            model.id = OPENAI_GPT_5_3;
            model.name = tr("GPT-5.3 Chat");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaFunctionCalling | ModelAbility::MaImage);
            model.modelId = "gpt-5.3-chat-latest";

            openai.models.append(model.id);
            m_models.insert(model.id, model);
        }

        m_providers.insert(openai.id, openai);
    }
#endif
    // 火山引擎
    {
        ProviderInfo volcengine;
        volcengine.id = STR_KEY_VOLCENGINE;
        volcengine.name = tr("doubao/seed");//tr("VolcanoEngine");

        //doubao-seed 2.0
        {
            ModelInfo model;
            model.id = VOLCENGINE_DOOUBAO_SEED_2_0;
            model.name = tr("Doubao-Seed 2.0");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaImage | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "doubao-seed-2-0-pro-260215";

            volcengine.models.append(model.id);
            m_models.insert(model.id, model);
        }

        //doubao-seed 2.0-code
        {
            ModelInfo model;
            model.id = VOLCENGINE_DOOUBAO_SEED_2_0_CODE;
            model.name = tr("Doubao-Seed 2.0-Code");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaImage | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "doubao-seed-2-0-code-preview-260215";

            volcengine.models.append(model.id);
            m_models.insert(model.id, model);
        }

        m_providers.insert(volcengine.id, volcengine);
    }

    // DeepSeek
    {
        ProviderInfo deepseek;
        deepseek.id = STR_KEY_DEEPSEEK;
        deepseek.name = tr("DeepSeek");
        // deepseek v3.2
        {
            ModelInfo model;
            model.id = DEEPSEEK_DEEPSEEK_V3_2;
            model.name = tr("DeepSeek V3.2");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "deepseek-chat";

            deepseek.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // deepseek v4 flash
        {
            ModelInfo model;
            model.id = DEEPSEEK_DEEPSEEK_V4_FLASH;
            model.name = tr("DeepSeek V4 Flash");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "deepseek-v4-flash";

            deepseek.models.append(model.id);
            m_models.insert(model.id, model);
        }


        // deepseek v4 pro
        {
            ModelInfo model;
            model.id = DEEPSEEK_DEEPSEEK_V4_PRO;
            model.name = tr("DeepSeek V4 Pro");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "deepseek-v4-pro";

            deepseek.models.append(model.id);
            m_models.insert(model.id, model);
        }
        m_providers.insert(deepseek.id, deepseek);
    }

    // MiniMax
    {
        ProviderInfo minimax;
        minimax.id = STR_KEY_MINIMAX;
        minimax.name = tr("MiniMax");

        // MiniMax-M2.5
        {
            ModelInfo model;
            model.id = MINIMAX_MINIMAX_M2_5;
            model.name = tr("MiniMax-M2.5");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "MiniMax-M2.5";

            minimax.models.append(model.id);
            m_models.insert(model.id, model);
        }

        m_providers.insert(minimax.id, minimax);
    }

    // 月之暗面
    {
        ProviderInfo moonshot;
        moonshot.id = STR_KEY_MOONSHOT;
        moonshot.name = tr("Moonshot (KIMI)");
        // KIMI k2
        {
            ModelInfo model;
            model.id = MOONSHOT_KIMI_2;
            model.name = tr("Kimi K2");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall);
            model.modelId = "kimi-k2-0905-preview";

            moonshot.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // KIMI k2.5
        {
            ModelInfo model;
            model.id = MOONSHOT_KIMI_2_5;
            model.name = tr("Kimi K2.5");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning | ModelAbility::MaImage);
            model.modelId = "kimi-k2.5";

            moonshot.models.append(model.id);
            m_models.insert(model.id, model);
        }

        m_providers.insert(moonshot.id, moonshot);
    }

    // 智谱
    {
        ProviderInfo bigmodel;
        bigmodel.id = STR_KEY_BIGMODEL;
        bigmodel.name = tr("GLM");
        // GLM 4.7
        {
            ModelInfo model;
            model.id = BIGMODEL_GLM_4_7;
            model.name = tr("GLM-4.7");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "glm-4.7";

            bigmodel.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // GLM 5.0
        {
            ModelInfo model;
            model.id = BIGMODEL_GLM_5;
            model.name = tr("GLM-5");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "glm-5";

            bigmodel.models.append(model.id);
            m_models.insert(model.id, model);
        }

        m_providers.insert(bigmodel.id, bigmodel);
    }

    // 阿里云百炼
    {   
        ProviderInfo bailian;
        bailian.id = STR_KEY_BAILIAN;
        bailian.name = tr("Qwen");

        // Qwen 3
        {
            ModelInfo model;    
            model.id = BAILIAN_QWEN_3;
            model.name = tr("Qwen3");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaReasoning);
            model.modelId = "qwen3-max";

            bailian.models.append(model.id);
            m_models.insert(model.id, model);
        }

        // Qwen3.5
        {
            ModelInfo model;    
            model.id = BAILIAN_QWEN_3_5;
            model.name = tr("Qwen3.5");
            model.arch = MaLanguage;
            model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall | ModelAbility::MaImage | ModelAbility::MaReasoning);
            model.modelId = "qwen3.5-plus";

            bailian.models.append(model.id);
            m_models.insert(model.id, model);
        }

        m_providers.insert(bailian.id, bailian);
    }

#if 0
    // Anthropic
    {
        ProviderInfo anthropic;
        anthropic.id = STR_KEY_ANTHROPIC;
        anthropic.name = tr("Anthropic");
    }

    // Gemini
    {
        ProviderInfo gemini;
        gemini.id = STR_KEY_GEMINI;
        gemini.name = tr("Gemini");
    }
#endif
}

void BuiltinProvider::initializeExternalProviders()
{
    QString content = AppDatabase::instance()->getConfig(CONFIG_EXTERNAL_MODELS);
    if (content.isEmpty()) {
        return;
    }

    {
        UosAccountEncoder coder(UOS_EXTERNAL_MODEL_PASSWD);
        content = std::get<0>(coder.decrypt(content));
    }

    if (content.isEmpty()) {
        qCWarning(logModel) << "Failed to decrypt external model json.";
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return;
    }
    
    auto root = doc.object();
    const QString locale = QLocale::system().name().simplified();

    // 解析 models 数组
    if (root.contains("models") && root["models"].isArray()) {
        QJsonArray modelsArray = root["models"].toArray();
        
        for (QJsonValue modelValue : modelsArray) {
            if (!modelValue.isObject()) {
                continue;
            }
            
            QJsonObject modelObj = modelValue.toObject();
            ModelInfo model;
            
            // 解析模型ID
            model.id = modelObj.value("id").toString();
            if (model.id.isEmpty()) {
                qCWarning(logModel) << "External model ID is empty: " << modelObj;
                continue;
            }
            
            // 解析模型名称（支持国际化）
            {
                QJsonObject nameObj = modelObj.value("name").toObject();
                model.name = nameObj.value(locale).toString();
                if (model.name.isEmpty()) {
                    model.name = nameObj.value("default").toString();
                }

                if (model.name.isEmpty()) {
                    qCWarning(logModel) << "External model name is empty: " << modelObj;
                    continue;
                }
            }
           
           // 解析模型架构
           model.arch = static_cast<ModelArch>(modelObj.value("arch").toInt());

           // 解析模型能力
           model.ability = ModelAbilities(modelObj.value("ability").toInt());

            // 解析模型ID（平台特定的模型标识）
            model.modelId = modelObj.value("mode_id").toString();
            if (model.modelId.isEmpty()) {
                qCWarning(logModel) << "External model mode_id is empty: " << modelObj;
                continue;
            }
            
            QString provider = modelObj.value("provider").toString().toLower();
            if (!m_providers.contains(provider) || model.id.compare(STR_KEY_UOS_AI) == 0) {
                qCWarning(logModel) << "External model provider not found: " << modelObj;
                continue;
            }
            
            auto proObj = m_providers.find(provider);

            if (m_models.contains(model.id)) {
                auto oldModel = m_models.value(model.id);

                if (!proObj->models.contains(model.id)) {
                    qCWarning(logModel) << "External model ID is duplicated in different provider: " 
                                         << modelObj << " -> " << oldModel.name;
                    continue;
                }

                qCInfo(logModel) << "External model replace " << oldModel.name << oldModel.modelId << "to" 
                                    << modelObj;
                m_models.insert(model.id, model);;
            } else {
                qCInfo(logModel) << "External model add " << modelObj;
                proObj->models.append(model.id);
                m_models.insert(model.id, model);
            }
        }
    }
}


