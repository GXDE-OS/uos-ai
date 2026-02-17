// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yzsbsplugin.h"
#include "yzsbsllm.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

using namespace uos_ai;
using namespace yzsbs;

YzsbsPlugin::YzsbsPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList YzsbsPlugin::modelList() const
{
    return {YzsbsLLM::modelID()};
}

QStringList YzsbsPlugin::roles(const QString &model) const
{
    if (model != YzsbsLLM::modelID())
        return {};

    return {YzsbsLLM::role()};
}

QVariant YzsbsPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == YzsbsLLM::modelID())
            return "UOS LM";
        else if (id == YzsbsLLM::role())
            return QString("一站式交付");
    } else if (query == QUERY_ICON_NAME) {
        if (id == YzsbsLLM::role())
            return "yzsbs";
        else if(id == YzsbsLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/uoslm.svg";
    } else if (query == QUERY_DESCRIPTION) {
        if (id == YzsbsLLM::role())
            return QString("解答关于《统信一站式桌面交付解决方案》相关问题");
    } else if (query == QUERY_QUESTIONS) {
        if (id == YzsbsLLM::role()) {
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

LLMModel *YzsbsPlugin::createModel(const QString &name)
{
    if (name == YzsbsLLM::modelID())
        return new YzsbsLLM;

    return nullptr;
}

void YzsbsPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-yzsbs-inquiry.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open xxzsk FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "xxzsk FAQ file format error!";
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
