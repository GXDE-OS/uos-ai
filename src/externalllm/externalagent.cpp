// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "externalagent.h"
#include "global_key_define.h"
#include "tas/uosaccountencoder.h"

#include <QFile>
#include <QLoggingCategory>
#include <QVariantHash>
#include <QLocale>
#include <QJsonArray>
#include <QDir>
#include <QJsonDocument>

using namespace uos_ai;

Q_DECLARE_LOGGING_CATEGORY(logExternalLLM)

static QString splitLocaleName(const QString &locale)
{
    QString ret;

    //去掉_后的本地语言标识
    QStringList localeList = locale.split("_");
    if (localeList.size() == 2 && !localeList.first().isEmpty())
        ret = localeList.first();
    return ret;
}

static QString readLocale(const QVariantHash &obj, const QString &locale, const QString &localeSplited)
{
    QString ret = obj.value(locale).toString();
    if (!ret.isEmpty())
        return ret;

    ret = obj.value(localeSplited).toString();
    if (!ret.isEmpty())
        return ret;

    return obj.value("default").toString();
}

static QString restoreKey(const QString &base)
{
    if (base.isEmpty())
        return base;

    UosAccountEncoder coder("2ef2fbf9265038a9");
    QString value = std::get<0>(coder.decrypt(base));
    return value.isEmpty() ? base : value;
}

ExternalAgent::ExternalAgent() : LLMPlugin()
{

}

QSharedPointer<ExternalAgent> ExternalAgent::fromJson(const QString &file, const QVariantHash &root)
{
    QSharedPointer<ExternalAgent> tmp(new ExternalAgent);
    const QString locale = QLocale::system().name().simplified();
    const QString localeSplited = splitLocaleName(locale);

    qCDebug(logExternalLLM) << "Creating ExternalAgent from JSON configuration";
    
    tmp->name = readLocale(root.value("name").toHash(), locale, localeSplited);
    if (tmp->name.isEmpty()) {
        qCWarning(logExternalLLM) << "Agent name is empty in configuration";
        return nullptr;
    }

    tmp->agentId = root.value("id").toString();
    if (tmp->agentId.isEmpty()) {
        qCWarning(logExternalLLM) << "Agent ID is empty in configuration";
        return nullptr;
    }

    tmp->description = readLocale(root.value("description").toHash(), locale, localeSplited);
    tmp->iconName = root.value("iconName").toString();
    tmp->faqFile = readLocale(root.value("question").toHash(), locale, localeSplited);
    tmp->prefix = root.value("resource").toString();
    if (tmp->prefix.isEmpty()) {
        qCWarning(logExternalLLM) << "Resource prefix is empty in configuration";
        return nullptr;
    }

    // models
    {
        qCDebug(logExternalLLM) << "Processing models configuration";
        for (const QVariant &v : root.value("model").toList()) {
            auto m = v.toHash();

            ModelAccountPtr acc(new ModelAccount);
            QString mid = m.value("id").toString();
            acc->id = QString::fromUtf8(QCryptographicHash::hash(file.toUtf8(), QCryptographicHash::Md5).toHex()) + "#" + mid;
            acc->model.name = readLocale(m.value("name").toHash(), locale, localeSplited);
            acc->account.provider = provider(m.value("type").toString());
            acc->network = STR_KEY_ONLINE;
            acc->model.arch = MaLanguage;
            acc->model.ability = ModelAbilities(ModelAbility::MaText);

            if (mid.isEmpty() || acc->model.name.isEmpty() || acc->account.provider.isEmpty())
                continue;

            const QVariantHash &api = m.value("api").toHash();
            if (acc->account.provider.compare(STR_KEY_OPENAI_COMPATIBLE, Qt::CaseInsensitive) == 0) {
                QString url = api.value("url").toString();
                QString apiKey = restoreKey(api.value("key").toString());

                acc->account.additional.insert(STR_KEY_PROVIDER_HOST, url);
                acc->account.auth.insert(STR_KEY_API_KEY, apiKey);
                acc->model.modelId = api.value("model").toString();

                if (url.isEmpty() || apiKey.isEmpty()) {
                    qCWarning(logExternalLLM) << "Invalid API configuration for model" << acc->model.name
                                             << "- URL:" << url << "API Key:" << (apiKey.isEmpty() ? "empty" : "present");
                    continue;
                }
            } else if (acc->account.provider.compare(STR_KEY_COZE_AGENT, Qt::CaseInsensitive) == 0) {
                acc->account.auth = api;
            }

            tmp->models.insert(acc->id, acc);
            QString icon = m.value("iconName").toString();
            if (!icon.isEmpty())
                tmp->modelIcon.insert(acc->id, icon);
        }

        if (tmp->models.isEmpty()) {
            qCWarning(logExternalLLM) << "No valid models found in configuration";
            return nullptr;
        }
    }

    qCInfo(logExternalLLM) << "Successfully created ExternalAgent:" << tmp->name << "with" << tmp->models.size() << "models";
    return tmp;
}

QString ExternalAgent::provider(const QString &strType)
{
    static const QHash<QString, QString> modelMap =
    {
        {"coze", STR_KEY_COZE_AGENT}, {"openai", STR_KEY_OPENAI_COMPATIBLE}
    };

    return modelMap.value(strType.toLower(), "");
}

QStringList ExternalAgent::roles(const QString &model) const
{
    if (modelList().contains(model))
        return {agentId};

    return {};
}

QVariant ExternalAgent::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == agentId)
            return name;
        else if (models.contains(id)) {
            return models.value(id)->model.name;
        }
    } else if (query == QUERY_ICON_NAME) {
        if (id == agentId)
            return iconName;
        else if (modelIcon.contains(id)) {
            QDir dir(prefix + "/icons");
            return dir.absoluteFilePath(modelIcon.value(id));
        }
    } else if (query == QUERY_DESCRIPTION) {
        if (id == agentId)
            return description;
    } else if (query == QUERY_ICON_PREFIX) {
        return "file://" + prefix + "/icons/";
    } else if (query == QUERY_QUESTIONS) {
        if (id == agentId) {
            QFile file(prefix + "/question/" + faqFile);
            if (file.open(QFile::ReadOnly)) {
                QByteArray faqData = file.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(faqData);
                QJsonArray array;
                if (doc.isArray()) {
                    for (const QJsonValue &v : doc.array()) {
                        auto vh = v.toObject().toVariantHash();
                        QString str = vh.value("question").toString();
                        if (str.isEmpty())
                            continue;

                        QJsonObject obj;
                        obj.insert("iconName", vh.value("iconName").toString());
                        obj.insert("Question", str);
                        obj.insert("iconPrefix", "file://" + prefix + "/icons/");
                        array.append(obj);
                    }
                }
                return QVariant(QJsonDocument(array).toJson());
            }
        }
    }

    return QVariant();
}

QStringList ExternalAgent::modelList() const
{
    return models.keys();
}

LLMModel *ExternalAgent::createModel(const QString &name)
{
    return nullptr;
}
