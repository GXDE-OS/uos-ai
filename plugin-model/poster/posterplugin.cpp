// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "posterplugin.h"
#include "posterllm.h"

#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

using namespace uos_ai;

static const char *kRolePC = "Poster Assistant"; //海报助手

PosterPlugin::PosterPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList PosterPlugin::modelList() const
{
    // 返回模型名字
    return {PosterLLM::modelID()};
}

QStringList PosterPlugin::roles(const QString &model) const
{
    if (model != PosterLLM::modelID())
        return {};

    return {kRolePC};
}

QVariant PosterPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == PosterLLM::modelID())
            return PosterLLM::modelID();
        else if (id == kRolePC)
            return QString("海报助手");
    } else if (query == QUERY_ICON_NAME) {
        if(id == PosterLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/yixin ai.svg";
        else if (id == kRolePC)
            return "poster-assistant";
        else
            return QVariant();
    } else if (query == QUERY_DESCRIPTION) {
        if (id == kRolePC)
            return "一句话即可为您生成海报";
        else
            return QVariant();
    } else if (query == QUERY_ICON_PREFIX) {
        return "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
    } else if (query == QUERY_QUESTIONS) {
        if (id == kRolePC) {
            QJsonArray jsonArray;
            for (const QJsonObject &obj : m_posterCreateFAQ) {
                jsonArray.append(obj);
            }
            QJsonDocument doc(jsonArray);
            QByteArray faqData = doc.toJson(QJsonDocument::Compact);
            return QVariant(faqData);
        }  else {
            return QVariant();
        }
    }

    return QVariant();
}

LLMModel *PosterPlugin::createModel(const QString &name)
{
    if (name == PosterLLM::modelID())
        return new PosterLLM;
    return nullptr;
}

void PosterPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-ai-poster.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open regulatory-inquiry FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "ai-poster FAQ file format error!";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_posterCreateFAQ.append(faqObject);
        }
    }
}
