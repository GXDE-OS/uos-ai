// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ztbplugin.h"
#include "ztbllm.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

using namespace uos_ai;
using namespace ztb;

ZtbPlugin::ZtbPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList ZtbPlugin::modelList() const
{
    return {ZtbLLM::modelID()};
}

QStringList ZtbPlugin::roles(const QString &model) const
{
    if (model != ZtbLLM::modelID())
        return {};

    return {ZtbLLM::role()};
}

QVariant ZtbPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == ZtbLLM::modelID())
            return ZtbLLM::modelID();
        else if (id == ZtbLLM::role())
            return QString("招投标宝典");
    } else if (query == QUERY_ICON_NAME) {
        if (id == ZtbLLM::role())
            return "ztbbd";
        else if(id == ZtbLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/uoslm.svg";
    } else if (query == QUERY_DESCRIPTION) {
        if (id == ZtbLLM::role())
            return QString("为您解答招投标相关的问题。");
    } else if (query == QUERY_QUESTIONS) {
        if (id == ZtbLLM::role()) {
            QJsonArray jsonArray;
            for (const QJsonObject &obj : m_faq)
                jsonArray.append(obj);

            QJsonDocument doc(jsonArray);
            QByteArray faqData = doc.toJson(QJsonDocument::Compact);
            return QVariant(faqData);
        }
    } else if (query == QUERY_ICON_PREFIX) {
        return "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
    }

    return QVariant();
}

LLMModel *ZtbPlugin::createModel(const QString &name)
{
    if (name == ZtbLLM::modelID())
        return new ZtbLLM;

    return nullptr;
}

void ZtbPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-ztbbd-inquiry.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open ztbbd FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "ztbbd FAQ file format error!";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_faq.append(faqObject);
        }
    }
}
