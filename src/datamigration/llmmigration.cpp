#include "llmmigration.h"
#include "database/modelstable.h"
#include "database/providertable.h"
#include "database/appdatabase.h"
#include "global_key_define.h"
#include "model/modelinfo.h"
#include "builtinprovider.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QFile>

Q_DECLARE_LOGGING_CATEGORY(logMigration)

using namespace uos_ai;

QString LlmMigration::name() const
{
    return "llm_migration";
}

bool LlmMigration::isNeeded() const
{
    return true;
}

bool LlmMigration::migrate()
{
    return migrateLlmData();
}

bool LlmMigration::migrateLlmData()
{
    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/db";
    QString oldDbPath = dbDir + "/basic";
    QString newDbPath = dbDir + "/app";
    
    
    AppDatabase *newDb = AppDatabase::instance();
    
    static const QString suffix = QStringLiteral("/chat/completions");
    QMap<QString, ProviderAccount> providers;
    QMap<QString, ModelAccountPtr> accounts;
    
    {
        QSqlDatabase oldDb = QSqlDatabase::addDatabase("QSQLITE", "llm_migration_old");
        oldDb.setDatabaseName(oldDbPath);

        if (!oldDb.open()) {
            qCCritical(logMigration) << "Failed to open old database:" << oldDb.lastError().text();
            return false;
        }

        QSqlQuery query(oldDb);
        if (!query.exec("SELECT uuid, name, type, desc, account_proxy, ext FROM llm")) {
            qCCritical(logMigration) << "Failed to query old llm data:" << query.lastError().text();
            oldDb.close();
            QSqlDatabase::removeDatabase("llm_migration_old");
            return false;
        }

        while (query.next()) {
            QString apiKey;
            int type = query.value("type").toInt();
            QString name = query.value("name").toString();

            {
                QString proxy = query.value("account_proxy").toString();
                if (!proxy.isEmpty()) {
                    QJsonDocument accountDoc = QJsonDocument::fromJson(proxy.toUtf8());
                    QJsonObject accountObj = accountDoc.object();
                    if (accountObj.contains("apiKey")) {
                        apiKey = accountObj["apiKey"].toString();
                    }
                }
            }

            if (apiKey.isEmpty()) {
                qCWarning(logMigration) << "Invalid data for type" << type << ", missing api_key:" << name;
                continue;
            }

            if (type == 1 || type == 2 || type == 3 || type == 4) {
                // 暂支持迁移gpt模型, 打印日志
                qCInfo(logMigration) << " unsupport to migrating GPT model:" << name << "type:" << type;
                continue;
                // CHATGPT_3_5          = 1,   // GPT3.5
                // CHATGPT_3_5_16K      = 2,   // GPT3.5 16k
                // CHATGPT_4            = 3,   // GPT4
                // CHATGPT_4_32K        = 4,   // GPT4 32k
                ProviderAccount acc;
                if (!providers.contains(apiKey)) {
                    acc.id = GlobalUtil::generateUuid();
                    acc.name = QObject::tr("OpenAI");
                    acc.provider = STR_KEY_OPENAI;
                    acc.auth.insert(STR_KEY_API_KEY, apiKey);
                    providers.insert(apiKey, acc);
                } else {
                    acc = providers.value(apiKey);
                }

                ModelAccountPtr modelAcc(new ModelAccount);
                modelAcc->id = GlobalUtil::generateUuid();
                modelAcc->account.id = acc.id;
                modelAcc->model.id = (type == 1 || type == 2) ? OPENAI_GPT_3_5 : OPENAI_GPT_4;
                accounts.insert(modelAcc->id, modelAcc);
                continue;
            } else if (type == 80) {
                // DeepSeek_R1
                ProviderAccount acc;
                if (!providers.contains(apiKey)) {
                    acc.id = GlobalUtil::generateUuid();
                    acc.name = QObject::tr("DeepSeek");
                    acc.provider = STR_KEY_DEEPSEEK;
                    acc.auth.insert(STR_KEY_API_KEY, apiKey);
                    providers.insert(apiKey, acc);
                } else {
                    acc = providers.value(apiKey);
                }

                ModelAccountPtr modelAcc(new ModelAccount);
                modelAcc->id = GlobalUtil::generateUuid();
                modelAcc->account.id = acc.id;
                modelAcc->model.id = DEEPSEEK_DEEPSEEK_V3_2;
                accounts.insert(modelAcc->id, modelAcc);
                continue;
            } else if (type == 81 || type == 82) {
                // 免费模型只需保留一个
                ProviderAccount acc;
                if (!providers.contains(STR_KEY_UOS_AI)) {
                    acc.id = GlobalUtil::generateUuid();
                    acc.name = QObject::tr("UOS AI Trial Account");
                    acc.provider = STR_KEY_UOS_AI;
                    acc.auth.insert(STR_KEY_API_KEY, apiKey);
                    providers.insert(STR_KEY_UOS_AI, acc);
                } else {
                    acc = providers.value(STR_KEY_UOS_AI);
                }

                ModelAccountPtr modelAcc(new ModelAccount);
                modelAcc->id = GlobalUtil::generateUuid();
                modelAcc->account.id = acc.id;
                modelAcc->model.id = UOS_FREE_MODEL_AUTO;
                accounts.insert(modelAcc->id, modelAcc);
                continue;
            } else if (type == 2000) {
                QJsonDocument extDoc = QJsonDocument::fromJson(query.value("ext").toByteArray());
                QJsonObject extObj = extDoc.object().value("ext").toObject();

                QString vendorUrl;
                QString vendorModel;
                if (extObj.contains("vendor_url")) {
                    vendorUrl = extObj["vendor_url"].toString();
                    if (vendorUrl.endsWith(suffix)) {
                        vendorUrl = vendorUrl.left(vendorUrl.length() - suffix.length());
                    }
                }

                if (vendorUrl.isEmpty()) {
                    qCWarning(logMigration) << "Invalid data for type 2000, missing vendor_url:" << name;
                    continue;
                }

                if (extObj.contains("vendor_model")) {
                    vendorModel = extObj["vendor_model"].toString();
                }

                if (vendorModel.isEmpty()) {
                    qCWarning(logMigration) << "Invalid data for type 2000, missing vendor_model:" << name;
                    continue;
                }

                QString providerKey = apiKey + "|" + vendorUrl;
                ProviderAccount acc;
                if (!providers.contains(providerKey)) {
                    acc.id = GlobalUtil::generateUuid();
                    acc.name = QObject::tr("Custom");
                    acc.provider = STR_KEY_OPENAI_COMPATIBLE;
                    acc.auth.insert(STR_KEY_API_KEY, apiKey);
                    acc.additional.insert(STR_KEY_PROVIDER_HOST, vendorUrl);
                    providers.insert(providerKey, acc);
                } else {
                    acc = providers.value(providerKey);
                }

                ModelAccountPtr modelAcc(new ModelAccount);
                modelAcc->id = GlobalUtil::generateUuid();
                modelAcc->account.id = acc.id;
                modelAcc->model.name = name;
                modelAcc->model.arch = MaLanguage;
                modelAcc->model.modelId = vendorModel;
                modelAcc->model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall);
                accounts.insert(modelAcc->id, modelAcc);
                continue;
            } else if (type == 4000) {
                QJsonDocument extDoc = QJsonDocument::fromJson(query.value("ext").toByteArray());
                QJsonObject extObj = extDoc.object().value("ext").toObject();

                QString vendorUrl;
                QString vendorModel;

                if (extObj.contains("vendor_url")) {
                    vendorUrl = extObj["vendor_url"].toString();
                    if (vendorUrl.endsWith(suffix)) {
                        vendorUrl = vendorUrl.left(vendorUrl.length() - suffix.length());
                    }
                }

                if (vendorUrl.isEmpty()) {
                    qCWarning(logMigration) << "Invalid data for type 2000, missing vendor_url:" << name;
                    continue;
                }

                if (extObj.contains("vendor_model")) {
                    vendorModel = extObj["vendor_model"].toString();
                }

                if (vendorModel.isEmpty()) {
                    qCWarning(logMigration) << "Invalid data for type 2000, missing vendor_model:" << name;
                    continue;
                }

                QString providerKey = apiKey + "|" + vendorUrl;
                ProviderAccount acc;
                if (!providers.contains(providerKey)) {
                    acc.id = GlobalUtil::generateUuid();
                    acc.name = QObject::tr("Private deployment model");
                    acc.provider = STR_KEY_PRIVATE_NET;
                    acc.auth.insert(STR_KEY_API_KEY, apiKey);
                    acc.additional.insert(STR_KEY_PROVIDER_HOST, vendorUrl);
                    providers.insert(providerKey, acc);
                } else {
                    acc = providers.value(providerKey);
                }

                ModelAccountPtr modelAcc(new ModelAccount);
                modelAcc->id = GlobalUtil::generateUuid();
                modelAcc->account.id = acc.id;
                modelAcc->model.name = name;
                modelAcc->model.arch = MaLanguage;
                modelAcc->model.modelId = vendorModel;
                modelAcc->model.ability = ModelAbilities(ModelAbility::MaText | ModelAbility::MaToolCall);
                accounts.insert(modelAcc->id, modelAcc);
                continue;
            } else {
                qCInfo(logMigration) << "ignore unsupported model:" << type << ", name:" << name;
                continue;
            }
        }

        oldDb.close();
    }

    QSqlDatabase::removeDatabase("llm_migration_old");

    for (const auto &acc : providers) {
        QJsonObject authObj;
        for (auto it = acc.auth.constBegin(); it != acc.auth.constEnd(); ++it) {
            authObj[it.key()] = QJsonValue::fromVariant(it.value());
        }

        QJsonObject additionalObj;
        for (auto it = acc.additional.constBegin(); it != acc.additional.constEnd(); ++it) {
            additionalObj[it.key()] = QJsonValue::fromVariant(it.value());
        }

        ProviderTable provider = ProviderTable::create(
            acc.id,
            acc.name,
            QJsonDocument(authObj).toJson(QJsonDocument::Compact),
            acc.provider,
            QJsonDocument(additionalObj).toJson(QJsonDocument::Compact)
        );
        qCInfo(logMigration) << "Save provider:" << acc.id << acc.name << acc.provider;
        provider.save(newDb);
    }

    for (const auto &modelAcc : accounts) {
        QJsonObject modelObj;
        if (modelAcc->model.id.isEmpty()) {
            modelObj = ModelsTable::createModel(static_cast<int>(modelAcc->model.arch), static_cast<int>(modelAcc->model.ability)
                                                , modelAcc->model.modelId, modelAcc->model.name);
        } else {
            modelObj = ModelsTable::createModel(modelAcc->model.id);
        }

        QJsonObject paramsObj;
        for (auto it = modelAcc->params.constBegin(); it != modelAcc->params.constEnd(); ++it) {
            paramsObj[it.key()] = QJsonValue::fromVariant(it.value());
        }

        ModelsTable model = ModelsTable::create(
            modelAcc->id,
            modelAcc->account.id,
            modelObj,
            paramsObj,
            {}
        );
        qCInfo(logMigration) << "Save model:" << modelAcc->id << modelAcc->account.id;
        model.save(newDb);
    }

    qCInfo(logMigration) << "LLM migration completed. Success:" << providers.size() << "providers," << accounts.size() << "models";
    return true;
}
