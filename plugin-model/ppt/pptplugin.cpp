// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pptplugin.h"
#include "pptllm.h"

#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

using namespace uos_ai;

static const char *kRolePC = "PPT Assistant"; //ppt助手

PPTPlugin::PPTPlugin(QObject *parent)
    : QObject(parent)
    , LLMPlugin()
{
    loadFAQ();
}

QStringList PPTPlugin::modelList() const
{
    // 返回模型名字
    return {PPTLLM::modelID()};
}

QStringList PPTPlugin::roles(const QString &model) const
{
    if (model != PPTLLM::modelID())
        return {};

    return {kRolePC};
}

QVariant PPTPlugin::queryInfo(const QString &query, const QString &id)
{
    if (query == QUERY_DISPLAY_NAME) {
        if (id == PPTLLM::modelID())
            return PPTLLM::modelID();
        else if (id == kRolePC)
            return QString("AI PPT 助手");
    } else if (query == QUERY_ICON_NAME) {
        if(id == PPTLLM::modelID())
            return QString(ASSETS_INSTALL_DIR) + "icons/llm-ppt.svg";
        else if (id == kRolePC)
            return "PPT-assistant";
        else
            return QVariant();
    } else if (query == QUERY_DESCRIPTION) {
        if (id == kRolePC)
            return "根据您输入的文本、上传的文件或者Markdown大纲，生产PPT。";
        else
            return QVariant();
    } else if (query == QUERY_ICON_PREFIX) {
        return "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
    } else if (query == QUERY_QUESTIONS) {
        if (id == kRolePC) {
            QJsonArray jsonArray;
            for (const QJsonObject &obj : m_pptcreateFAQ) {
                jsonArray.append(obj);
            }
            QJsonDocument doc(jsonArray);
            QByteArray faqData = doc.toJson(QJsonDocument::Compact);
            return QVariant(faqData);
        }  else {
            return QVariant();
        }
    } else if (query == QUERY_INSTLIST){
        QJsonArray resultArray;

        QJsonObject instObj1;
        instObj1.insert("tagType", 999);
        instObj1.insert("tagName", "大纲生成PPT");
        instObj1.insert("content", "请将 markdown 的大纲粘贴在此处，具体格式要求如下，该模式下不会修改大纲的内容：\n#PPT标题\n##章节标题\n###内容页标题\n####正文标题\n-正文内容\n-正文内容");
        instObj1.insert("description", "根据大纲生产PPT，大纲需要符合固定格式，如果格式不符合，需要提示修改格式");

        resultArray.append(instObj1);

        return QJsonDocument(resultArray).toJson(QJsonDocument::Compact);
    }

    return QVariant();
}

LLMModel *PPTPlugin::createModel(const QString &name)
{
    if (name == PPTLLM::modelID())
        return new PPTLLM;
    return nullptr;
}

void PPTPlugin::loadFAQ()
{
    QFile file(QString(ASSETS_INSTALL_DIR) + "assistant-ai-ppt.json");
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open regulatory-inquiry FAQ file.";
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (!jsonDoc.isArray()) {
        qWarning() << "ai-ppt FAQ file format error!";
        return;
    }

    QJsonArray jsonArray = jsonDoc.array();
    foreach (const QJsonValue &value, jsonArray) {
        if (value.isObject()) {
            QJsonObject faqObject = value.toObject();
            faqObject["iconPrefix"] = "file://" + QString(ASSETS_INSTALL_DIR) + "icons/";
            m_pptcreateFAQ.append(faqObject);
        }
    }
}
