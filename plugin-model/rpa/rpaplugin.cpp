#include "rpaplugin.h"
#include "rpallm.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

using namespace uos_ai;
using namespace uos_ai::rpa;

RpaPlugin::RpaPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList RpaPlugin::modelList() const
{
    return {RpaLLM::modelID()};
}

QStringList RpaPlugin::roles(const QString &model) const
{
    if (model != RpaLLM::modelID())
        return {};

    return {RpaLLM::role()};
}

QVariant RpaPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == RpaLLM::modelID())
            return QString("百度千帆");
        else if (id == RpaLLM::role())
            return QString("RPA助手");
    } else if (query == QUERY_ICON_NAME) {
        if(id == RpaLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/baidu.svg";
        else if (id == RpaLLM::role())
            return "RPAassistant";
        else
            return QVariant();
    } else if (query == QUERY_DESCRIPTION) {
        if (id == RpaLLM::role())
            return "智能流程，⼀键提效。";
        else
            return QVariant();
    } else if (query == QUERY_ICON_PREFIX) {
        return "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
    } else if (query == QUERY_QUESTIONS) {
        if (id == RpaLLM::role()) {
            QJsonArray jsonArray;
            for (const QJsonObject &obj : m_rpaFAQ) {
                jsonArray.append(obj);
            }
            QJsonDocument doc(jsonArray);
            QByteArray faqData = doc.toJson(QJsonDocument::Compact);
            return QVariant(faqData);
        } else {
            return QVariant();
        }
    }
    return QVariant();
}

LLMModel *RpaPlugin::createModel(const QString &name)
{
    if (name == RpaLLM::modelID())
        return new RpaLLM;

    return nullptr;
}

void RpaPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-ai-rpa.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open rpa FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "ai-rpa FAQ file format error!";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_rpaFAQ.append(faqObject);
        }
    }
}
