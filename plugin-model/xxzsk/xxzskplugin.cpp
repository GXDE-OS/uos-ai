// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "xxzskplugin.h"
#include "xxzskllm.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

using namespace uos_ai;
using namespace xxzsk;

XxzskPlugin::XxzskPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList XxzskPlugin::modelList() const
{
    return {XxzskLLM::modelID()};
}

QStringList XxzskPlugin::roles(const QString &model) const
{
    if (model != XxzskLLM::modelID())
        return {};

    return {XxzskLLM::role()};
}

QVariant XxzskPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == XxzskLLM::modelID())
            return "UOS LM";
        else if (id == XxzskLLM::role())
            return QString("行销知识库");
    } else if (query == QUERY_ICON_NAME) {
        if (id == XxzskLLM::role())
            return "ztbbd";
        else if(id == XxzskLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/uoslm.svg";
    } else if (query == QUERY_DESCRIPTION) {
        if (id == XxzskLLM::role())
            return QString("为您解答行销知识库相关的问题。");
    } else if (query == QUERY_QUESTIONS) {
        if (id == XxzskLLM::role()) {
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

LLMModel *XxzskPlugin::createModel(const QString &name)
{
    if (name == XxzskLLM::modelID())
        return new XxzskLLM;

    return nullptr;
}

void XxzskPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-xxzsk-inquiry.json");
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
